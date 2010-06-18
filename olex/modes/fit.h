#ifndef __OLX_FIT_MODE_H
#define __OLX_FIT_MODE_H
#include "xgroup.h"
#include "match.h"

class TFitMode : public AMode  {
  TXGroup* group;
  TXAtomPList Atoms, AtomsToMatch;
protected:
public:
  TFitMode(size_t id) : AMode(id)  {}
  bool Initialise(TStrObjList& Cmds, const TParamList& Options) {
    AtomsToMatch.Clear();
    TGlXApp::GetMainForm()->SetUserCursor('0', "<F>");
    group = &TGlXApp::GetGXApp()->GetRender().ReplaceSelection<TXGroup>();
    return true;
  }
  void Finalise() {
    TGXApp& app = *TGlXApp::GetGXApp();
    TAsymmUnit& au = app.XFile().GetAsymmUnit();
    RefinementModel& rm = app.XFile().GetRM();
    for( size_t i=0; i < Atoms.Count(); i++ )  {
      Atoms[i]->Atom().CAtom().ccrd() = Atoms[i]->Atom().crd();
      Atoms[i]->Atom().ccrd() = au.CartesianToCell(Atoms[i]->Atom().CAtom().ccrd());
      rm.Vars.FixParam(Atoms[i]->Atom().CAtom(), catom_var_name_Sof);
    }
    app.GetRender().ReplaceSelection<TGlGroup>();
    app.XFile().GetLattice().UpdateConnectivity();
  }
  virtual bool OnObject(AGDrawObject &obj)  {
    if( EsdlInstanceOf(obj, TXAtom) )  {
      if( AtomsToMatch.IsEmpty() && Atoms.IndexOf((TXAtom&)obj) == InvalidIndex )
        return true;
      AtomsToMatch.Add((TXAtom&)obj);
      TGlXApp::GetMainForm()->SetUserCursor(AtomsToMatch.Count(), "<F>");
      if( (AtomsToMatch.Count()%2) == 0 )  {
        TMatchMode::FitAtoms(AtomsToMatch, "<F>", false);
        group->UpdateRotationCenter();
      }
    }
    return true;
  }
  virtual void OnGraphicsDestroy()  {
    Atoms.Clear();
    AtomsToMatch.Clear();
    TGlXApp::GetMainForm()->SetUserCursor('0', "<F>");
  }
  virtual bool OnKey(int keyId, short shiftState)  {
    if( shiftState == 0 && keyId == WXK_ESCAPE )  {
      if( AtomsToMatch.IsEmpty() )  return false;
      AtomsToMatch.Delete(AtomsToMatch.Count()-1);
      TGlXApp::GetMainForm()->SetUserCursor(AtomsToMatch.Count(), "<F>");
      return true;
    }
    return false;
  }
  virtual bool AddAtoms(const TXAtomPList& atoms)  {
    Atoms.AddList(atoms);
    group->AddAtoms(atoms);
    group->SetSelected(true);
    return true;
  }
};

#endif
