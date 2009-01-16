#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#include "envilist.h"

#ifndef _NO_PYTHON
PyObject* TAtomEnvi::PyExport(TPtrList<PyObject>& atoms)  {
  PyObject* main = PyDict_New();
  Py_IncRef( atoms[Base->GetTag()] );
  PyDict_SetItemString(main, "base", atoms[Base->GetTag()]); 
  PyObject* neighbours = PyTuple_New( Envi.Count() );
  for( int i=0; i < Envi.Count(); i++ )  {
    PyObject* atom = atoms[Envi[i].GetA()->GetTag()];
    const smatd& mat = Envi[i].GetB();
    const vec3d& crd = Envi[i].GetC();
    Py_IncRef(atom);
    PyTuple_SetItem(neighbours, i, 
      Py_BuildValue("OOO", atom,
        Py_BuildValue("(ddd)", crd[0], crd[1], crd[2]),
          Py_BuildValue("(ddd)(ddd)(ddd)(ddd)", 
            mat.r[0][0], mat.r[0][1], mat.r[0][2],
            mat.r[1][0], mat.r[1][1], mat.r[1][2],
            mat.r[2][0], mat.r[2][1], mat.r[2][2],
            mat.t[0], mat.t[1], mat.t[2] )
       )
    );
  }
  PyDict_SetItemString(main, "neighbours", neighbours); 
  return main;
}
#endif

 
