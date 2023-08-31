import sys, importlib


def import_force(m):
    if m not in sys.modules:
        return __import__(m)
    else:
        raise ImportError("module {} with the same name is already imported".format(m))
