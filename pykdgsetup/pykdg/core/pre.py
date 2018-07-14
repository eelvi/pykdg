import pykdgc

class ktre_match:
    def __init__(self, ktre_c_object):
        self.__c_obj__ = ktre_c_object
    def group(self, n):
        return self.__c_obj__.__group__(n)
    def groups(self):
        return self.__c_obj__.__groups__()
    def __bool__(self):
        return self.__c_obj__.__bool__()
def search(src, pat, opt=''):
    return ktre_match( pykdgc.__search(src,pat,opt) )


