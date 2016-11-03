from fnmatch import fnmatch
from importlib import import_module
from integration_test import IntegrationTest
import inspect
import os

def is_integration_test(test_class):
	return inspect.isclass(test_class) and issubclass(test_class, IntegrationTest)

for file in os.listdir("."):
	if fnmatch(file, "test_*.py"):
		test_case_module_name = file.replace(".py", "")
		test_case_module = __import__(test_case_module_name)
		classed_defined_in_module = inspect.getmembers(test_case_module, is_integration_test)
		test_case_name, test_case = next(defined_class for defined_class in classed_defined_in_module if defined_class is not IntegrationTest)
		print "Running:", test_case_name
		test_case().runTest()