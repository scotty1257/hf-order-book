project = "order-book"
extensions = ["breathe", "exhale"]

# Breathe reads the XML that Doxygen writes.
breathe_projects = {"project": "../build/doxygen/xml"}
breathe_default_project = "project"

# Exhale writes one .rst page per type into docs/api on every build.
# gen_docs.sh runs Doxygen itself, so Exhale doesn't need to.
exhale_args = {
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "rootFileTitle": "API Reference",
    "doxygenStripFromPath": "..",
    "createTreeView": True,
    "exhaleExecutesDoxygen": False,
}

primary_domain = "cpp"
highlight_language = "cpp"

html_theme = "furo"
html_title = "order-book"
