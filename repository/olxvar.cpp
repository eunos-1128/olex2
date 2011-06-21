#include "olxvar.h"
#include "egc.h"

TOlxVars* TOlxVars::Instance = NULL;
#ifndef _NO_PYTHON
#include "pyext.h"
char TOlxPyVar::svmn[] = "setValue";
char TOlxPyVar::gvmn[] = "getValue";

TOlxPyVar::TOlxPyVar(PyObject* obj)  {
  Str = NULL;
  Obj = NULL;
  InitObject(obj);

}
//..............................................................................
TOlxPyVar::TOlxPyVar(const TOlxPyVar& oo)  {
  Type = oo.Type;
  Str = (oo.Str == NULL) ? NULL : new olxstr(*oo.Str);
  Obj = oo.Obj;
  if( Obj != NULL )  Py_INCREF(Obj);
}
//..............................................................................
TOlxPyVar::~TOlxPyVar()  {
  if( Str != NULL )  delete Str;
  if( Obj != NULL && Obj != Py_None)
    Py_DECREF(Obj);
}
//..............................................................................
void TOlxPyVar::InitObject(PyObject* obj)  {
  if( Obj != NULL )  Py_DECREF(Obj);
  Obj = obj;
  Py_INCREF(obj);

  Type = potNone;

  if( PyObject_HasAttrString(obj, gvmn) )  Type |= potHasGetter;
  if( PyObject_HasAttrString(obj, svmn) )  Type |= potHasSetter;

  PyObject* defv = NULL;
  PyTypeObject *ot;

  if( HasGet() )  {
    defv = PyObject_CallMethod(Obj, gvmn, NULL);
    ot = defv->ob_type;
  }
  else
    ot = obj->ob_type;

  if( ot == &PyInt_Type )          Type |= potInt;
  else if( ot == &PyBool_Type )    Type |= potBool;
  else if( ot == &PyFloat_Type )   Type |= potFloat;
  else if( ot == &PyString_Type )  Type |= potString;
  else if( ot == &PyUnicode_Type ) Type |= potString;
  else if( ot == &PyComplex_Type ) Type |= potComplex;
  else  Type |= potUndefined;

  if( defv != NULL )  Py_DECREF(defv);
}
//..............................................................................
PyObject* TOlxPyVar::ObjectValue(PyObject* obj)  {
  return PyObject_HasAttrString(obj, svmn) ? PyObject_CallMethod(obj, gvmn, NULL) : obj;
}
//..............................................................................
PyObject* TOlxPyVar::GetObjVal()  {
  if( Obj != NULL )  {
    PyObject *rv = HasGet() ? PyObject_CallMethod(Obj, gvmn, NULL) : Obj;
    if( !HasGet() )
      Py_INCREF(rv);
    return rv;
  }
  else  if( Str != NULL )
    return PythonExt::BuildString(*Str);
  else
    return PythonExt::SetErrorMsg(PyExc_RuntimeError, __OlxSourceInfo, "Uninitialised object");
}
//..............................................................................
void TOlxPyVar::Set(PyObject* obj)  {
  if( Obj == NULL && obj == Py_None )  {
    Py_DECREF(PythonExt::SetErrorMsg(PyExc_TypeError, __OlxSourceInfo, "A valid object is expected"));
    return;
//    throw TInvalidArgumentException(__OlxSourceInfo, "A valid object is expected");
  }
  if( Obj != NULL )  {
    if( obj == Py_None )  {
      if( !HasSet() )  {
        Py_DECREF(PythonExt::SetErrorMsg(PyExc_TypeError, __OlxSourceInfo, "Missing set method"));
        return;
      }
    }
    if( Obj->ob_type == obj->ob_type )  {
      Py_DECREF(Obj);
      Py_INCREF(obj);
      Obj = obj;
    }
    else if( !HasSet() )  // replace object type
      InitObject(obj);
    else  {
      PyObject *rv = PyObject_CallMethod(Obj, svmn, "O", obj);
      if( rv != NULL )  Py_DECREF(rv);
    }
  }
  else  {
    if( obj->ob_type == &PyString_Type || obj->ob_type == &PyUnicode_Type )
      *Str = PythonExt::ParseStr(obj);
    else  {
      Py_DECREF(PythonExt::SetErrorMsg(PyExc_RuntimeError, __OlxSourceInfo, "Uninitialised object"));
      return;
//      throw TInvalidArgumentException(__OlxSourceInfo, "Uninitialised object");
    }
  }
}
//..............................................................................
void TOlxPyVar::Set(const olxstr& str)  {
  if( Obj == NULL )  {
    if( Str == NULL )  Str = new olxstr(str);
    else               *Str = str;
  }
  else  {
    if( HasSet() )  {
      PyObject *arg = PythonExt::BuildString(str);
      PyObject *rv = PyObject_CallMethod(Obj, svmn, "O", arg);
      if( rv != NULL )  Py_DECREF(rv);
      Py_DECREF(arg);
    }
    else  {  // replace a primitive value with new object
      PyObject * arg;
      if( (Type & potBool) != 0  )  {
        bool v = str.Equalsi(TrueString());
        if( !v && !str.Equalsi(FalseString()) )  {
          olxstr err(olxstr("Boolean is expected, got '") << str << '\'');
          Py_DECREF(PythonExt::SetErrorMsg(PyExc_TypeError, __OlxSourceInfo, err));
          throw TInvalidArgumentException(__OlxSourceInfo, err);
        }
        arg = PyBool_FromLong(v);
      }
      else if( (Type & potInt) != 0  )  {
        if( !str.IsNumber() )  {
          olxstr err(olxstr("Boolean is expected, got '") << str << '\'');
          Py_DECREF(PythonExt::SetErrorMsg(PyExc_TypeError, __OlxSourceInfo, err));
          throw TInvalidArgumentException(__OlxSourceInfo, err);
        }
        arg = Py_BuildValue("i", str.ToInt());
      }
      else if( (Type & potFloat) != 0 )  {
        if( !str.IsNumber() )  {
          olxstr err(olxstr("Boolean is expected, got '") << str << '\'');
          Py_DECREF(PythonExt::SetErrorMsg(PyExc_TypeError, __OlxSourceInfo, err));
          throw TInvalidArgumentException(__OlxSourceInfo, err);
        }
        arg = Py_BuildValue("d", str.ToDouble());
      }
      else if( (Type & potString) != 0 )
        arg = PythonExt::BuildString(str);
      else  {
        olxstr err(olxstr("Boolean is expected, got '") << str << '\'');
        Py_DECREF(PythonExt::SetErrorMsg(PyExc_TypeError, __OlxSourceInfo, err));
        throw TInvalidArgumentException(__OlxSourceInfo, err);
      }
      Py_DECREF(Obj);
      Obj = arg;
    }
  }
}
//..............................................................................
const olxstr& TOlxVars::GetVarStr(size_t index)  {
  TOlxPyVar& oo = Instance->Vars.GetObject(index);
  if( oo.GetStr() != NULL )  return *oo.GetStr();
  PyObject *po = oo.GetObjVal();
  double fv;
  if( po->ob_type == &PyString_Type || po->ob_type == &PyUnicode_Type )
    return TEGC::New<olxstr>( PythonExt::ParseStr(po) );
  else if( po->ob_type == &PyFloat_Type && PyArg_Parse(po, "d", &fv) )
    return TEGC::New<olxstr>(fv);
  else
    return TEGC::New<olxstr>(PyObject_REPR(po));
}
//..............................................................................
TOlxVars::TOlxVars()  {
  if( Instance != NULL )
    throw TFunctionFailedException(__OlxSourceInfo, "singleton");
  Instance = this;
  OnVarChange = &Actions.New("OnVarChange");
//  TEGC::AddP(this);  // we cannot do this, as Pyhon might be Finalised boforehad!!!
}
#endif // _NO_PYTHON

#ifdef __BORLANDC__
  #pragma package(smart_init)
#endif
