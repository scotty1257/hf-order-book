#!/usr/bin/env python3

project = "Order Book"
extensions = ["breathe"]

breathe_projects = {"OrderBook": "../build/doxygen/xml"}
breathe_default_project = "OrderBook"
breathe_default_members = ("members", "undoc-members")

html_theme = "furo"
