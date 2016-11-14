# -*- coding: utf-8 -*-

from fnmatch import fnmatch
from importlib import import_module
from integration_test import IntegrationTest
import inspect
import os
import sys
import subprocess

os.system("rm -rf testResults")
os.system("mkdir testResults")

for file in os.listdir("."):
	if fnmatch(file, "test_*.py"):
		test_case_name = file.replace(".py", "")
		print "Running", test_case_name + "..."
		test_status = os.system("python {}.py  > testResults/{}.log".format(test_case_name, test_case_name))
		if not test_status == 0:
			raise AssertionError("Test {} Failed!".format(test_case_name))
		else:
			print "  Test", test_case_name, "PASSED! :)"

print "All integration tests passed! :) ✅"
