/*
 *      
 *      almost everything in these bindings was borrowed from functions in the kdg library
 *      This module is highly unstable, it's very experiemental at the moment
 *
 *
 *
 *
 *
 *   resources that helped in creating this:
 *       python's c api reference:
 *         https://docs.python.org/3.7/c-api/
 *       python's type c api guide
 *         https://ref.readthedocs.io/en/latest/understanding_python/type_system/PyTypeObject.html
 *      yizhang's post in microsoft's blog:
 *        https://blogs.msdn.microsoft.com/yizhang/2018/01/27/calling-c-functions-from-python-part-2-writing-cpython-extensions-using-pythonc-api/
 */


#include <Python.h>
#include <string.h>
#include "kdgu.h"
#include "ktre.h"

	
const char *
encoding_str(enum fmt f)
{
    switch(f){
     case  KDGU_FMT_CP1252 : return "CP1252";
     case  KDGU_FMT_EBCDIC : return "EBCDIC";
     case  KDGU_FMT_ASCII  : return "ASCII";
     case  KDGU_FMT_UTF8   : return "UTF-8";
     case  KDGU_FMT_UTF16  : return "UTF-16";
     case  KDGU_FMT_UTF16BE: return "UTF-16-BE";
     case  KDGU_FMT_UTF16LE: return "UTF-16-LE";
     case  KDGU_FMT_UTF32  : return "UTF-32";
     case  KDGU_FMT_UTF32LE: return "UTF-32-LE";
     case  KDGU_FMT_UTF32BE : return "UTF-32-BE";
     default : return "invalid";
    }

}


kdgu *
subgroup(kdgu *src, int **vec, int match, int group)
{
    //borrowed from ktre.c
    kdgu *substr = kdgu_substr(src, 
                               vec[match][group * 2], 
                               vec[match][group * 2] + vec[match][group * 2 + 1]);
    return substr;
}

static PyTypeObject ktre_type;

struct _ktre_data{
    int match;
    kdgu *src;
    kdgu *pat;
    ktre *kt;
    int **vec;
    int opt;
};


//this thing contains a refrence count and type* in addition to our data below it
// it is what each instance will point to
typedef struct {
    PyObject_HEAD
    struct _ktre_data data;

} ktre_object;

typedef struct _ktre_data ktre_data;



static PyObject *ktre_match_new_c(ktre_data *data);


static PyObject *
pykdg_search(PyObject *self, PyObject *args)
{
    //prototype is src, pat, opt
    const char *py_str1, *py_str2, *py_str3;
    ktre_data data;
    int opt = 0;

    if (!PyArg_ParseTuple(args, "sss", &py_str1, &py_str2, &py_str3)){
        return NULL;
    }

    while( *py_str3 ){
        switch(*(py_str3++)){
            case 'i': opt |= KTRE_INSENSITIVE; break;
            case 'u': opt |= KTRE_UNANCHORED ; break;
            case 'x': opt |= KTRE_EXTENDED   ; break;
            case 'g': opt |= KTRE_GLOBAL     ; break;
            case 'm': opt |= KTRE_MULTILINE  ; break;
            case 'c': opt |= KTRE_CONTINUE   ; break;
            case 'd': opt |= KTRE_DEBUG      ; break;
            case 'e': opt |= KTRE_ECMA       ; break;
        }
    }


    data.src = kdgu_new(KDGU_FMT_UTF8, py_str1, strlen(py_str1));
    data.pat = kdgu_new(KDGU_FMT_UTF8, py_str2, strlen(py_str2));


    if ((!data.src) || (!data.pat)){
        if (data.src)
            kdgu_free(data.src);
        if (data.pat)
            kdgu_free(data.pat);

        /* fprintf(file, "k's fault\n"); */
        PyErr_SetString(PyExc_RuntimeError, "failed to load the provided strings.");
        return NULL;
    }

    data.kt = ktre_compile(data.pat, opt);

    if (!data.kt || (data.kt->err)) {
        kdgu_free(data.src);
        kdgu_free(data.src);
        if(data.kt)
            ktre_free(data.kt);
        PyErr_SetString(PyExc_RuntimeError, "an error occured in ktre.");
        return NULL;
    }

    ktre_exec(data.kt, data.src, &(data.vec));
    data.match = data.kt->num_matches;

    return (PyObject *) ktre_match_new_c(&data);
}


static PyObject *
pykdg_group(PyObject *_self, PyObject *args){
    
    long s, req_group, num_groups;
    PyObject *group_str = NULL;
    kdgu *substr = NULL;
    ktre_object *self = (ktre_object *)_self;
    
    num_groups = self->data.kt->num_groups;
    if (!(self->data.match)){
        PyErr_SetString(PyExc_RuntimeError, "No match");
        return NULL;
    }
    s = PyArg_ParseTuple(args, "l",&req_group);
    if (!s || (req_group < 0) || (req_group >= num_groups)){;
        PyErr_SetString(PyExc_RuntimeError, "invalid group.");
        return NULL;
    }
    substr = subgroup(self->data.src, self->data.vec, 0, req_group);

    if (substr->fmt !=KDGU_FMT_UTF8 ){
        kdgu_free(substr); 
        PyErr_SetString(PyExc_RuntimeError, "wrong encoding, internal error in pykdg");
        return NULL;
    }
    group_str = PyUnicode_FromStringAndSize(substr->s, substr->len);  
    
    kdgu_free(substr); 
    return group_str;
}

static PyObject *
pykdg_groups(PyObject *_self, PyObject *args){
    ktre_object *self = (ktre_object *)_self;
    int n = self->data.kt->num_groups;
    return PyLong_FromLong(n - 1);
}
static PyObject *
pykdg_did_match(PyObject *_self, PyObject *args){
    ktre_object *self = (ktre_object *)_self;
    return PyBool_FromLong(self->data.match);
}
static PyObject *
pykdg_return_33(PyObject *self, PyObject *args){
    return PyLong_FromLong(33L);
}

static PyMethodDef ktre_methods[] = {
    { "__group__", (PyCFunction) pykdg_group, METH_VARARGS, "get group" },
    { "__groups__", (PyCFunction) pykdg_groups, METH_VARARGS, "no. groups" },
    { "__bool__", (PyCFunction) pykdg_did_match, METH_VARARGS, "__bool__ method" },
    { "return_33", (PyCFunction) pykdg_return_33, METH_VARARGS, "dbg" },
    { NULL }
};


//used inside this module, just copies our part of the ktre_object and returns a valid PyObject
static PyObject *
ktre_match_new_c(ktre_data *data)
{
    ktre_object *self;

    self = (ktre_object *)PyType_GenericAlloc(&ktre_type, 0);
    memcpy((char *) &(self->data), data, sizeof(ktre_data));

    return (PyObject *)self;
}


static PyObject *
ktre_match_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_RuntimeError, "unsupported constructor.");
    return NULL;
}

static PyObject *
ktre_match_dealloc(ktre_object *self){
     
    ktre_data *data = &(self->data); 
    kdgu_free(data->pat);  
    kdgu_free(data->src);
    ktre_free(data->kt );
    PyObject_Del(self);
}

//file level struct instance, this describes to python our type
static PyTypeObject ktre_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pykdg.ktre_match",        /* tp_name */
    sizeof(ktre_object),       /* tp_basicsize */
    0,                         /* tp_itemsize */
    ktre_match_dealloc,        /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "ktre match object",       /* tp_doc */    
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    ktre_methods,              /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                        /* tp_alloc */
    ktre_match_new,            /* tp_new */
};



static PyMethodDef pykdg_methods[] = {
    {"__search", pykdg_search , METH_VARARGS, "Hmm."},
};

static struct PyModuleDef pykdgmodule = {
    PyModuleDef_HEAD_INIT,
    "pykdgc",
    NULL,
    -1,
    pykdg_methods
};


PyMODINIT_FUNC
PyInit_pykdgc(void)
{
    if (PyType_Ready(&ktre_type) < 0){
            return NULL;
    }

    return PyModule_Create(&pykdgmodule);
}

