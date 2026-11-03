#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

// GoogleTest < 1.12 has no GTEST_FLAG_GET.
#if defined(GTEST_FLAG_GET)
#define PRETTY_GTEST_FLAG(name) GTEST_FLAG_GET(name)
#else
#define PRETTY_GTEST_FLAG(name) ::testing::GTEST_FLAG(name)
#endif

namespace pretty_gtest {

  struct ReporterOptions {
    // nullopt follows --gtest_color / GTEST_COLOR (and NO_COLOR when "auto").
    std::optional<bool> color;
    bool unicode = true;
    // Durations at or above this are yellow; at 5x they are red and listed under "Slowest".
    std::chrono::milliseconds slow_threshold{200};
    int slowest_test_count = 5;
    std::FILE *stream = stdout;
  };

  namespace detail {

    using ::testing::TimeInMillis;

    inline int file_descriptor(std::FILE *stream) {
#if defined(_WIN32)
      return _fileno(stream);
#else
      return fileno(stream);
#endif
    }

    inline bool is_terminal(std::FILE *stream) {
#if defined(_WIN32)
      return _isatty(file_descriptor(stream)) != 0;
#else
      return isatty(file_descriptor(stream)) != 0;
#endif
    }

    inline std::optional<int> parse_positive_int(std::string_view text) {
      int value = 0;
      const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
      if (error != std::errc{} || end != text.data() + text.size() || value <= 0)
        return std::nullopt;
      return value;
    }

    inline int terminal_columns(std::FILE *stream) {
#if defined(_WIN32)
      const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(file_descriptor(stream)));
      CONSOLE_SCREEN_BUFFER_INFO info{};
      if (handle != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(handle, &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
      }
#else
      winsize size{};
      if (ioctl(file_descriptor(stream), TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        return size.ws_col;
#endif
      if (const char *columns = std::getenv("COLUMNS")) {
        if (const auto parsed = parse_positive_int(columns)) return *parsed;
      }
      return 100;
    }

    // Windows consoles need VT processing switched on before ANSI escapes render.
    inline bool enable_virtual_terminal(std::FILE *stream) {
#if defined(_WIN32)
      const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(file_descriptor(stream)));
      DWORD mode = 0;
      if (handle == INVALID_HANDLE_VALUE || !GetConsoleMode(handle, &mode)) return false;
      if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) return true;
      return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
      (void)stream;
      return true;
#endif
    }

    inline bool equals_ignore_case(std::string_view left, std::string_view right) {
      return std::ranges::equal(left, right, [](char a, char b) {
        const auto lower = [](char c) {
          return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        };
        return lower(a) == lower(b);
      });
    }

    inline bool resolve_color(const ReporterOptions &options) {
      if (options.color) {
        if (*options.color && is_terminal(options.stream)) enable_virtual_terminal(options.stream);
        return *options.color;
      }

      const std::string gtest_color = PRETTY_GTEST_FLAG(color);
      for (const std::string_view forced_on : {"yes", "true", "t", "1"}) {
        if (equals_ignore_case(gtest_color, forced_on)) {
          if (is_terminal(options.stream)) enable_virtual_terminal(options.stream);
          return true;
        }
      }
      if (!equals_ignore_case(gtest_color, "auto")) return false;

      if (const char *no_color = std::getenv("NO_COLOR"); no_color != nullptr && *no_color != '\0')
        return false;
      if (const char *term = std::getenv("TERM");
          term != nullptr && std::string_view{term} == "dumb")
        return false;
      if (!is_terminal(options.stream)) return false;
      return enable_virtual_terminal(options.stream);
    }

    inline bool is_continuation_byte(char byte) {
      return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
    }

    // Code points, not grapheme clusters; good enough for test identifiers.
    inline std::size_t display_width(std::string_view text) {
      return static_cast<std::size_t>(std::ranges::count_if(text, [](char byte) {
        return !is_continuation_byte(byte);
      }));
    }

    inline std::string truncate_to_width(std::string_view text,
        std::size_t max_width,
        std::string_view ellipsis) {
      if (display_width(text) <= max_width) return std::string{text};
      const std::size_t ellipsis_width = display_width(ellipsis);
      if (max_width <= ellipsis_width) return std::string{ellipsis.substr(0, max_width)};

      const std::size_t keep_width = max_width - ellipsis_width;
      std::size_t width = 0;
      std::size_t cut = 0;
      for (; cut < text.size(); ++cut) {
        if (is_continuation_byte(text[cut])) continue;
        if (width == keep_width) break;
        ++width;
      }
      return std::string{text.substr(0, cut)} + std::string{ellipsis};
    }

    inline std::vector<std::string_view> split_lines(std::string_view text) {
      std::vector<std::string_view> lines;
      while (!text.empty()) {
        const auto newline = text.find('\n');
        lines.push_back(text.substr(0, newline));
        if (newline == std::string_view::npos) break;
        text.remove_prefix(newline + 1);
      }
      while (!lines.empty() && lines.back().empty())
        lines.pop_back();
      return lines;
    }

    inline std::string format_duration(TimeInMillis milliseconds) {
      if (milliseconds < 1000) return std::format("{} ms", milliseconds);
      if (milliseconds < 60'000)
        return std::format("{:.2f} s", static_cast<double>(milliseconds) / 1000.0);
      return std::format("{}m {:02}s", milliseconds / 60'000, (milliseconds / 1000) % 60);
    }

    inline std::string shell_quote(std::string_view text) {
#if defined(_WIN32)
      return std::format("\"{}\"", text);
#else
      return std::format("'{}'", text);
#endif
    }

    struct Glyphs {
      std::string_view pass;
      std::string_view fail;
      std::string_view skip;
      std::string_view gutter;
      std::string_view rule;
      std::string_view leader;
      std::string_view separator;
      std::string_view ellipsis;
    };

    inline constexpr Glyphs unicode_glyphs{"✓", "✗", "○", "│", "─", "·", " · ", "…"};
    inline constexpr Glyphs ascii_glyphs{"+", "x", "-", "|", "-", ".", ", ", "..."};

    namespace ansi {
      inline constexpr std::string_view bold = "1";
      inline constexpr std::string_view dim = "2";
      inline constexpr std::string_view red = "31";
      inline constexpr std::string_view green = "32";
      inline constexpr std::string_view yellow = "33";
      inline constexpr std::string_view cyan = "36";
      inline constexpr std::string_view bold_red = "1;31";
      inline constexpr std::string_view bold_green = "1;32";
      inline constexpr std::string_view pass_badge = "1;30;42";
      inline constexpr std::string_view fail_badge = "1;97;41";
    } // namespace ansi

    struct FailureRecord {
      std::string location;
      std::string message;
    };

  } // namespace detail

  class PrettyReporter final : public ::testing::EmptyTestEventListener {
  public:
    explicit PrettyReporter(ReporterOptions options = {})
        : options_(std::move(options)),
          glyphs_(options_.unicode ? detail::unicode_glyphs : detail::ascii_glyphs) {}

    PrettyReporter(const PrettyReporter &) = delete;
    PrettyReporter &operator=(const PrettyReporter &) = delete;

    void OnTestProgramStart(const ::testing::UnitTest &) override {
      // Flags are parsed by now, so the color decision is deferred to here rather than the
      // constructor.
      use_color_ = detail::resolve_color(options_);
      columns_ = std::clamp(detail::terminal_columns(options_.stream), 60, 120);

      std::error_code error;
      working_directory_ = std::filesystem::current_path(error);
      if (error) working_directory_.clear();

#if defined(_WIN32)
      if (options_.unicode && detail::is_terminal(options_.stream)) {
        previous_code_page_ = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
      }
#endif
    }

    void OnTestProgramEnd(const ::testing::UnitTest &) override {
#if defined(_WIN32)
      if (previous_code_page_ != 0) SetConsoleOutputCP(previous_code_page_);
#endif
    }

    void OnTestIterationStart(const ::testing::UnitTest &unit_test, int iteration) override {
      std::string out;
      if (PRETTY_GTEST_FLAG(repeat) != 1) {
        out += std::format(
            "\n{}\n", paint(std::format("Iteration {}", iteration + 1), detail::ansi::cyan));
      }

      out += std::format("\n{} {}",
          paint(std::format("Running {} {}", unit_test.test_to_run_count(),
                    plural(unit_test.test_to_run_count(), "test", "tests")),
              detail::ansi::bold),
          paint(std::format("from {} {}", unit_test.test_suite_to_run_count(),
                    plural(unit_test.test_suite_to_run_count(), "suite", "suites")),
              detail::ansi::dim));

      std::vector<std::string> run_details;
      if (const std::string filter = PRETTY_GTEST_FLAG(filter); filter != "*") {
        run_details.push_back(std::format("filter {}", filter));
      }
      if (PRETTY_GTEST_FLAG(shuffle))
        run_details.push_back(std::format("seed {}", unit_test.random_seed()));
      if (const auto shard = shard_description()) run_details.push_back(*shard);
      for (const auto &detail_text : run_details) {
        out += paint(std::format("{}{}", glyphs_.separator, detail_text), detail::ansi::dim);
      }
      out += '\n';
      write(out);
    }

    void OnTestSuiteStart(const ::testing::TestSuite &test_suite) override {
      flush_orphaned_failures("global environment set-up");

      std::string header = std::format("  {}", paint(test_suite.name(), detail::ansi::bold));
      if (const char *type_parameter = test_suite.type_param()) {
        header += paint(std::format(" <{}>", type_parameter), detail::ansi::cyan);
      }
      header += paint(std::format("  {} {}", test_suite.test_to_run_count(),
                          plural(test_suite.test_to_run_count(), "test", "tests")),
          detail::ansi::dim);
      pending_suite_header_ = header + '\n';
      // In brief mode the header waits until the suite has something worth showing.
      if (!PRETTY_GTEST_FLAG(brief)) emit({});
    }

    void OnTestStart(const ::testing::TestInfo &test_info) override {
      flush_orphaned_failures(std::format("{} set-up", test_info.test_suite_name()));
    }

    void OnTestPartResult(const ::testing::TestPartResult &part) override {
      if (part.type() == ::testing::TestPartResult::kSuccess) return;

      detail::FailureRecord record;
      if (part.type() != ::testing::TestPartResult::kSkip) record.location = source_location(part);
      record.message = part.message();

      // Assertions may fire from worker threads.
      const std::lock_guard lock(pending_mutex_);
      if (part.type() == ::testing::TestPartResult::kSkip) {
        pending_skip_reasons_.push_back(std::move(record.message));
      } else {
        pending_failures_.push_back(std::move(record));
      }
    }

    void OnTestEnd(const ::testing::TestInfo &test_info) override {
      auto [failures, skip_reasons] = take_pending();
      const ::testing::TestResult &result = *test_info.result();

      if (result.Passed() && !result.Skipped() && failures.empty() && PRETTY_GTEST_FLAG(brief))
        return;

      std::string out;
      if (result.Skipped()) {
        out +=
            test_line(test_info, paint(glyphs_.skip, detail::ansi::yellow), detail::ansi::yellow);
        for (const auto &reason : skip_reasons) {
          for (const auto line : detail::split_lines(reason)) {
            out += std::format("        {}\n", paint(line, detail::ansi::dim));
          }
        }
      } else if (result.Failed()) {
        out += test_line(test_info, paint(glyphs_.fail, detail::ansi::bold_red), detail::ansi::red);
        out += render_failures(failures);
      } else {
        out += test_line(test_info, paint(glyphs_.pass, detail::ansi::green), {});
      }
      emit(out);
    }

    void OnTestSuiteEnd(const ::testing::TestSuite &test_suite) override {
      flush_orphaned_failures(std::format("{} tear-down", test_suite.name()));
      pending_suite_header_.clear();
    }

    void OnTestIterationEnd(const ::testing::UnitTest &unit_test, int) override {
      flush_orphaned_failures("global environment");
      write(render_summary(unit_test));
    }

  private:
    using TimeInMillis = ::testing::TimeInMillis;

    ReporterOptions options_;
    detail::Glyphs glyphs_;
    bool use_color_ = false;
    bool ends_with_blank_line_ = false;
    std::string pending_suite_header_;
    int columns_ = 100;
    std::filesystem::path working_directory_;
#if defined(_WIN32)
    UINT previous_code_page_ = 0;
#endif

    std::mutex pending_mutex_;
    std::vector<detail::FailureRecord> pending_failures_;
    std::vector<std::string> pending_skip_reasons_;

    struct FailedEntry {
      std::string name;
      std::string location;
      std::string filter_pattern;
    };

    std::string paint(std::string_view text, std::string_view style) const {
      if (!use_color_ || style.empty()) return std::string{text};
      return std::format("\x1b[{}m{}\x1b[0m", style, text);
    }

    static std::string_view plural(int count,
        std::string_view singular,
        std::string_view plural_form) {
      return count == 1 ? singular : plural_form;
    }

    void write(std::string_view text) {
      if (text.empty()) return;
      std::fwrite(text.data(), 1, text.size(), options_.stream);
      ends_with_blank_line_ = text.ends_with("\n\n");
      // Keep ordering sane when the code under test also writes to stdout.
      std::fflush(options_.stream);
    }

    void emit(std::string_view text) {
      if (!pending_suite_header_.empty()) {
        write(std::format("{}{}", ends_with_blank_line_ ? "" : "\n", pending_suite_header_));
        pending_suite_header_.clear();
      }
      write(text);
    }

    std::pair<std::vector<detail::FailureRecord>, std::vector<std::string>> take_pending() {
      const std::lock_guard lock(pending_mutex_);
      return {std::exchange(pending_failures_, {}), std::exchange(pending_skip_reasons_, {})};
    }

    std::string relative_path(const char *file_name) const {
      if (file_name == nullptr) return "unknown";
      if (working_directory_.empty()) return file_name;
      const std::filesystem::path source{file_name};
      if (!source.is_absolute()) return source.generic_string();
      const auto relative = source.lexically_relative(working_directory_);
      if (relative.empty() || *relative.begin() == "..") return source.generic_string();
      return relative.generic_string();
    }

    std::string source_location(const ::testing::TestPartResult &part) const {
      if (part.file_name() == nullptr) return "unknown location";
      if (part.line_number() < 0) return relative_path(part.file_name());
      return std::format("{}:{}", relative_path(part.file_name()), part.line_number());
    }

    std::string duration_text(TimeInMillis milliseconds) const {
      const auto slow = options_.slow_threshold.count();
      const auto style = milliseconds >= slow * 5 ? detail::ansi::red
                         : milliseconds >= slow   ? detail::ansi::yellow
                                                  : detail::ansi::dim;
      return paint(detail::format_duration(milliseconds), style);
    }

    std::string test_line(const ::testing::TestInfo &test_info,
        const std::string &icon,
        std::string_view name_style) const {
      constexpr std::size_t indent_width = 4;
      constexpr std::size_t icon_width = 2;
      constexpr std::size_t duration_column = 12;

      const bool show_time = PRETTY_GTEST_FLAG(print_time);
      const std::string duration_plain =
          show_time ? detail::format_duration(test_info.result()->elapsed_time()) : std::string{};
      const std::size_t right_reserve = show_time ? duration_column : 0;
      const std::size_t available =
          static_cast<std::size_t>(columns_) - indent_width - icon_width - right_reserve;

      const std::string name =
          detail::truncate_to_width(test_info.name(), available, glyphs_.ellipsis);
      std::string left = paint(name, name_style);
      std::size_t left_width = detail::display_width(name);

      if (const char *value_parameter = test_info.value_param()) {
        const std::size_t room = available > left_width + 3 ? available - left_width - 3 : 0;
        if (room > 4) {
          const std::string hint =
              detail::truncate_to_width(value_parameter, room, glyphs_.ellipsis);
          left += paint(std::format(" [{}]", hint), detail::ansi::cyan);
          left_width += detail::display_width(hint) + 3;
        }
      }

      if (!show_time) return std::format("    {} {}\n", icon, left);

      const std::size_t used =
          indent_width + icon_width + left_width + 1 + detail::display_width(duration_plain);
      const std::size_t leader_count = used + 2 < static_cast<std::size_t>(columns_)
                                           ? static_cast<std::size_t>(columns_) - used - 2
                                           : 0;
      std::string leader;
      if (leader_count >= 3) {
        for (std::size_t index = 0; index < leader_count; ++index)
          leader += glyphs_.leader;
      }

      return std::format("    {} {} {} {}\n", icon, left, paint(leader, detail::ansi::dim),
          duration_text(test_info.result()->elapsed_time()));
    }

    std::string render_failures(const std::vector<detail::FailureRecord> &failures) const {
      const std::string gutter = paint(glyphs_.gutter, detail::ansi::red);
      std::string out;
      for (std::size_t index = 0; index < failures.size(); ++index) {
        const auto &failure = failures[index];
        if (index > 0) out += std::format("      {}\n", gutter);
        out += std::format("      {} {}\n", gutter, paint(failure.location, detail::ansi::cyan));
        for (const auto line : detail::split_lines(failure.message)) {
          out += std::format("      {} {}\n", gutter, line);
        }
      }
      if (!failures.empty()) out += '\n';
      return out;
    }

    // Failures recorded outside any test: SetUpTestSuite/TearDownTestSuite and global environments.
    void flush_orphaned_failures(std::string_view context) {
      auto [failures, skip_reasons] = take_pending();
      if (failures.empty()) return;
      std::string out = std::format("    {} {}\n", paint(glyphs_.fail, detail::ansi::bold_red),
          paint(context, detail::ansi::red));
      out += render_failures(failures);
      emit(out);
    }

    std::optional<std::string> shard_description() const {
      const char *total = std::getenv("GTEST_TOTAL_SHARDS");
      const char *index = std::getenv("GTEST_SHARD_INDEX");
      if (total == nullptr || index == nullptr) return std::nullopt;
      const auto total_shards = detail::parse_positive_int(total);
      if (!total_shards || *total_shards <= 1) return std::nullopt;
      return std::format("shard {}/{}", index, *total_shards);
    }

    static std::string first_failure_location(const ::testing::TestResult &result,
        const PrettyReporter &reporter) {
      for (int index = 0; index < result.total_part_count(); ++index) {
        const auto &part = result.GetTestPartResult(index);
        if (part.failed()) return reporter.source_location(part);
      }
      return {};
    }

    std::vector<FailedEntry> collect_failures(const ::testing::UnitTest &unit_test) const {
      std::vector<FailedEntry> failed;
      for (int suite_index = 0; suite_index < unit_test.total_test_suite_count(); ++suite_index) {
        const ::testing::TestSuite &suite = *unit_test.GetTestSuite(suite_index);
        if (!suite.should_run()) continue;

        if (suite.ad_hoc_test_result().Failed()) {
          failed.push_back({std::format("{} (suite fixture)", suite.name()),
              first_failure_location(suite.ad_hoc_test_result(), *this),
              std::format("{}.*", suite.name())});
        }
        for (int test_index = 0; test_index < suite.total_test_count(); ++test_index) {
          const ::testing::TestInfo &test_info = *suite.GetTestInfo(test_index);
          if (!test_info.should_run() || !test_info.result()->Failed()) continue;
          std::string full_name =
              std::format("{}.{}", test_info.test_suite_name(), test_info.name());
          failed.push_back(
              {full_name, first_failure_location(*test_info.result(), *this), full_name});
        }
      }
      if (unit_test.ad_hoc_test_result().Failed()) {
        failed.push_back({"global environment",
            first_failure_location(unit_test.ad_hoc_test_result(), *this), {}});
      }
      return failed;
    }

    struct SlowEntry {
      std::string name;
      TimeInMillis elapsed;
    };

    std::vector<SlowEntry> collect_slowest(const ::testing::UnitTest &unit_test) const {
      std::vector<SlowEntry> slowest;
      if (options_.slowest_test_count <= 0) return slowest;
      for (int suite_index = 0; suite_index < unit_test.total_test_suite_count(); ++suite_index) {
        const ::testing::TestSuite &suite = *unit_test.GetTestSuite(suite_index);
        for (int test_index = 0; test_index < suite.total_test_count(); ++test_index) {
          const ::testing::TestInfo &test_info = *suite.GetTestInfo(test_index);
          const TimeInMillis elapsed = test_info.result()->elapsed_time();
          if (!test_info.should_run() || elapsed < options_.slow_threshold.count()) continue;
          slowest.push_back(
              {std::format("{}.{}", test_info.test_suite_name(), test_info.name()), elapsed});
        }
      }
      const auto keep =
          std::min(slowest.size(), static_cast<std::size_t>(options_.slowest_test_count));
      std::ranges::partial_sort(slowest, slowest.begin() + static_cast<std::ptrdiff_t>(keep),
          std::ranges::greater{}, &SlowEntry::elapsed);
      slowest.resize(keep);
      return slowest;
    }

    std::string count_part(int count,
        std::string_view label,
        std::string_view style_when_nonzero) const {
      const std::string text = std::format("{} {}", count, label);
      return paint(text, count > 0 ? style_when_nonzero : detail::ansi::dim);
    }

    std::string render_summary(const ::testing::UnitTest &unit_test) const {
      std::string out = "\n  ";
      for (int index = 0; index < columns_ - 4; ++index)
        out += glyphs_.rule;
      out = paint(out, detail::ansi::dim) + "\n";

      const std::string separator = paint(glyphs_.separator, detail::ansi::dim);
      const auto label = [this](std::string_view text) {
        return paint(std::format("{:<8}", text), detail::ansi::dim);
      };

      std::string tests_line =
          count_part(unit_test.successful_test_count(), "passed", detail::ansi::green) + separator +
          count_part(unit_test.failed_test_count(), "failed", detail::ansi::bold_red);
      if (unit_test.skipped_test_count() > 0) {
        tests_line +=
            separator + count_part(unit_test.skipped_test_count(), "skipped", detail::ansi::yellow);
      }
      if (unit_test.reportable_disabled_test_count() > 0) {
        tests_line += separator + count_part(unit_test.reportable_disabled_test_count(), "disabled",
                                      detail::ansi::dim);
      }
      tests_line +=
          paint(std::format("  ({} ran)", unit_test.test_to_run_count()), detail::ansi::dim);

      out += std::format("  {}{}\n", label("Tests"), tests_line);
      out += std::format("  {}{}{}{}\n", label("Suites"),
          count_part(unit_test.successful_test_suite_count(), "passed", detail::ansi::green),
          separator,
          count_part(unit_test.failed_test_suite_count(), "failed", detail::ansi::bold_red));
      out +=
          std::format("  {}{}\n", label("Time"), detail::format_duration(unit_test.elapsed_time()));

      if (const auto slowest = collect_slowest(unit_test); !slowest.empty()) {
        out += std::format("\n  {}\n", paint("Slowest", detail::ansi::bold));
        for (const auto &entry : slowest) {
          const std::string duration = detail::format_duration(entry.elapsed);
          out += std::format("    {}{}  {}\n",
              std::string(9 - std::min<std::size_t>(9, duration.size()), ' '),
              duration_text(entry.elapsed), entry.name);
        }
      }

      const auto failed = collect_failures(unit_test);
      if (!failed.empty()) {
        out += std::format("\n  {}\n", paint("Failures", detail::ansi::bold_red));
        std::string rerun_filter;
        for (const auto &entry : failed) {
          out += std::format("    {} {}  {}\n", paint(glyphs_.fail, detail::ansi::red), entry.name,
              paint(entry.location, detail::ansi::dim));
          if (entry.filter_pattern.empty()) continue;
          if (!rerun_filter.empty()) rerun_filter += ':';
          rerun_filter += entry.filter_pattern;
        }
        if (!rerun_filter.empty()) {
          out += std::format(
              "\n  {}--gtest_filter={}\n", label("Rerun"), detail::shell_quote(rerun_filter));
        }
      }

      const bool passed = !unit_test.Failed();
      out += std::format("\n  {} {}\n\n",
          paint(passed ? " PASS " : " FAIL ",
              passed ? detail::ansi::pass_badge : detail::ansi::fail_badge),
          paint(passed ? "all tests passed" : std::format("{} failing", failed.size()),
              passed ? detail::ansi::bold_green : detail::ansi::bold_red));
      return out;
    }
  };

  // Call after ::testing::InitGoogleTest(): --gtest_brief swaps the default printer during flag
  // parsing, and installing earlier would leave both printers attached.
  inline void install(ReporterOptions options = {}) {
    ::testing::TestEventListeners &listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new PrettyReporter(std::move(options)));
  }

} // namespace pretty_gtest

#undef PRETTY_GTEST_FLAG
