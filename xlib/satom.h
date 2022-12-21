/******************************************************************************
* Copyright (c) 2004-2011 O. Dolomanov, OlexSys                               *
*                                                                             *
* This file is part of the OlexSys Development Framework.                     *
*                                                                             *
* This source file is distributed under the terms of the licence located in   *
* the root folder.                                                            *
******************************************************************************/

#ifndef __olx_xl_satom_H
#define __olx_xl_satom_H
#include "xbase.h"
#include "catom.h"
#include "symmat.h"
#include "typelist.h"
#include "tptrlist.h"
#include "comparable_operators.h"

BeginXlibNamespace()

const unsigned short
  satom_Deleted    = 0x0001,
  //satom_Grown      = 0x0002, - obsolete
  satom_Standalone = 0x0004,
  satom_Masked     = 0x0008,
  satom_Processed  = 0x0010;  // generic bit

class TSAtom : public TBasicNode<class TNetwork, TSAtom, class TSBond> {
  // the generator matrix, a reference borrowed from the Lattice
  const smatd *Matrix;
  // a list of pointers to matrices used for generation of atom
  TCAtom* FCAtom;  // basic crystallographic information
  class TEllipsoid* FEllipsoid;  // a pointer to TEllipsoid object
  vec3d  FCCenter;  // atom center in fractional coordinates
  vec3d  FCenter;  // atom center in cartesian coordinates
protected:
  mutable short Flags;
  int _SortNodesByDistanceAsc(const TSAtom &a1, const TSAtom &a2) const {
    return olx_cmp(
      FCenter.DistanceTo(a1.FCenter), FCenter.DistanceTo(a2.FCenter));
  }
  int _SortNodesByDistanceDsc(const TSAtom &a1, const TSAtom &a2) const {
    return olx_cmp(
      FCenter.DistanceTo(a2.FCenter), FCenter.DistanceTo(a1.FCenter));
  }
  static int _SortBondsByLengthAsc(const TSBond &b1, const TSBond &b2);
  static int _SortBondsByLengthDsc(const TSBond &b1, const TSBond &b2);
public:
  TSAtom(TNetwork* N);
  TSAtom(TNetwork* N, TCAtom::Site& site);
  TSAtom(TNetwork* N, class ExplicitCAtomRef& aref);
  virtual ~TSAtom()  {}
  void Assign(const TSAtom& S);
  // Is/Set
  virtual bool IsDeleted() const {  return  (Flags&satom_Deleted) != 0;  }
  virtual void SetDeleted(bool v)  {  olx_set_bit(v, Flags, satom_Deleted);  }
  DefPropBFIsSet(Standalone, Flags, satom_Standalone)
  DefPropBFIsSet(Masked, Flags, satom_Masked)
  DefPropBFIsSet(Processed, Flags, satom_Processed)

  bool IsAvailable() const {
    return (Flags&(satom_Deleted|satom_Masked)) == 0 && !FCAtom->IsDetached();
  }
  bool IsGrown() const {  return NodeCount() == CAtom().AttachedSiteCount();  }

  TCAtom& CAtom() const {  return *FCAtom;  }
  void CAtom(TCAtom& CA);

  const cm_Element& GetType() const {  return FCAtom->GetType(); }
  const olxstr& GetLabel() const {  return FCAtom->GetLabel(); }
  /* returns a label plus (if not identity) first matrix like
  label_resi.2_556
  */
  olxstr GetGuiLabel() const;
  /* returns a label plus (if not identity) first matrix like
  label_resi(-2/3+X,Y,2-Z)
  */
  olxstr GetGuiLabelEx() const;

  const smatd& GetMatrix() const {
#ifdef _DEBUG
    if (Matrix == 0) {
      throw TFunctionFailedException(__OlxSourceInfo, "uninitialised object");
    }
#endif
    return *Matrix;
  }
  // to be called by TLattice!
  void _SetMatrix(const smatd &M);
  // checks if the matrix is a generator for this atom site
  bool IsGenerator(const smatd &m) const { return IsGenerator(m.GetId()); }
  bool IsGenerator(uint32_t m_id) const;
  static double weight_unit(const TSAtom &)  {  return 1.0;  }
  static double weight_occu(const TSAtom &a)  {
    return a.CAtom().GetChemOccu();
  }
  static double weight_z(const TSAtom &a)  {  return a.GetType().z;  }
  static double weight_occu_z(const TSAtom &a)  {
    return a.CAtom().GetChemOccu()*a.GetType().z;
  }
  static double weight_atom_mass(const TSAtom &a)  {
    return a.GetType().GetMr()*a.CAtom().GetChemOccu();
  }
  static double weight_element_mass(const TSAtom &a)  {
    return a.GetType().GetMr();
  }
  /* returns true if the atom os generated by the identity transformation
  i.e. - belong to the asymmetric unit */
  bool IsAUAtom() const {  return GetMatrix().IsFirst();  }
  // beware that underlying objkect might be shared by several atoms!
  TEllipsoid* GetEllipsoid() const {  return FEllipsoid;  }
  void SetEllipsoid(TEllipsoid* v) {  FEllipsoid = v;  }
  vec3d& ccrd()  {  return FCCenter;  }
  vec3d& crd()  {  return FCenter;  }
  vec3d const& ccrd() const {  return FCCenter;  }
  vec3d const& crd() const {  return FCenter;  }
  // pointers comparison!
  bool operator == (const TSAtom& a) const {  return this == &a;  }
  class TLattice &GetParent() const;
  void SortNodesByDistanceAsc()  {
    QuickSorter::SortMF(Nodes, *this, &TSAtom::_SortNodesByDistanceAsc);
  }
  void SortNodesByDistanceDsc()  {
    QuickSorter::SortMF(Nodes, *this, &TSAtom::_SortNodesByDistanceDsc);
  }
  void SortBondsByLengthAsc()  {
    QuickSorter::SortSF(Bonds, &TSAtom::_SortBondsByLengthAsc);
  }
  void SortBondsByLengthDsc()  {
    QuickSorter::SortSF(Bonds, &TSAtom::_SortBondsByLengthDsc);
  }
  /* removes specified node from the list of nodes (this node is also removed
  from the list of the node nodes
  */
  void RemoveNode(TSAtom& node);

  struct Ref : public comparable_operators<Ref> {
    size_t au_id, atom_id;
    uint32_t matrix_id;
    Ref() : au_id(~0), atom_id(~0), matrix_id(~0)
    {}
    Ref(const TCAtom& a, uint32_t m_id);
    Ref(const Ref& r)
      : au_id(r.au_id), atom_id(r.atom_id), matrix_id(r.matrix_id)
    {}
    Ref(const TDataItem& item, const class TXApp& app) {
      FromDataItem(item, app);
    }
    Ref& operator = (const Ref& r) {
      au_id = r.au_id;
      atom_id = r.atom_id;
      matrix_id = r.matrix_id;
      return *this;
    }
    bool operator == (const Ref& r) const {
      return (au_id == r.au_id && atom_id == r.atom_id && matrix_id == r.matrix_id);
    }
    bool operator == (const TSAtom& a) const {
      return a.operator == (*this);
    }
    TCAtom& GetCAtom(const class TXApp& app) const;
    TLattice& GetLattice(const class TXApp& app) const;
    int Compare(const Ref& r) const;
    void ToDataItem(TDataItem& item, const TXApp& app, bool use_id=false) const;
    void FromDataItem(const TDataItem& item, const TXApp &app);
    void swap(Ref& r) {
      olx_swap(au_id, r.au_id);
      olx_swap(atom_id, r.atom_id);
      olx_swap(matrix_id, r.matrix_id);
    }
    // a string hash for the plane definition
    TIString ToString() const {
      olxstr_buf rv;
      return olxstr(rv << au_id << atom_id << matrix_id);
    }
  };

  bool operator == (const Ref& id) const;
  Ref GetRef() const;
  // finds the matrix with smallest Id
  static Ref GetRef(const TCAtom &a, const smatd &generator);

  virtual void ToDataItem(TDataItem& item) const;
  virtual void FromDataItem(const TDataItem& item, class TLattice& parent);

  // sorts atoms according to the distcance from {0,0,0}
  struct SortByDistance {
    template <class item_a_t, class item_b_t>
    int Compare(const item_a_t &A, const item_b_t &A1) const {
      return olx_cmp(olx_ref::get(A).crd().QLength(),
        olx_ref::get(A1).crd().QLength());
    }
  };

  template <class Accessor> struct FlagsAnalyser_ {
    const Accessor &accessor;
    const short ref_flags;
    FlagsAnalyser_(const Accessor &accessor_, short _ref_flags)
      : accessor(accessor_), ref_flags(_ref_flags)  {}
    template <class Item>
    bool OnItem(const Item& o, size_t) const {
      return (olx_ref::get(Accessor::Access(o)).Flags&ref_flags) != 0;
    }
  };
  template <class acc_t> static FlagsAnalyser_<acc_t>
  FlagsAnalyser(const acc_t &acc, short flag) {
    return FlagsAnalyser_<acc_t>(acc, flag);
  }
  static FlagsAnalyser_<DummyAccessor>
  FlagsAnalyser(short flags) {
    return FlagsAnalyser_<DummyAccessor>(DummyAccessor(), flags);
  }

  template <class Accessor> struct FlagSetter_ {
    const Accessor &accessor;
    const short ref_flags;
    bool set;
    FlagSetter_(const Accessor &accessor_, short ref_flags_, bool set_)
      : accessor(accessor_), ref_flags(ref_flags_), set(set_)
    {}
    template <class Item>
    void OnItem(Item& o, size_t) const {
      return olx_set_bit(set, olx_ref::get(accessor(o)).Flags, ref_flags);
    }
  };
  template <class acc_t> static FlagSetter_<acc_t>
  FlagSetter(const acc_t &acc, short ref_flags, bool set) {
    return FlagSetter_<acc_t>(acc, ref_flags, set);
  }
  static FlagSetter_<DummyAccessor>
  FlagSetter(short ref_flags, bool set) {
    return FlagSetter_<DummyAccessor>(DummyAccessor(), ref_flags, set);
  }

  template <class Accessor> struct TypeAnalyser_ {
    const Accessor &accessor;
    const short ref_type;
    TypeAnalyser_(const Accessor &accessor_, short _ref_type)
      : accessor(accessor_), ref_type(_ref_type)  {}
    template <class Item> bool OnItem(const Item& o, size_t) const {
      return olx_ref::get(accessor(o)).GetType() == ref_type;
    }
  };
  template <class acc_t> static TypeAnalyser_<acc_t>
  TypeAnalyser(const acc_t &acc, short z) {
    return TypeAnalyser_<acc_t>(acc, z);
  }
  template <class acc_t> static TypeAnalyser_<acc_t>
  TypeAnalyser(const acc_t &acc, const cm_Element &e) {
    return TypeAnalyser_<acc_t>(acc, e.z);
  }
  static TypeAnalyser_<DummyAccessor>
  TypeAnalyser(short z) {
    return TypeAnalyser_<DummyAccessor>(DummyAccessor(), z);
  }
  static TypeAnalyser_<DummyAccessor>
  TypeAnalyser(const cm_Element &e) {
    return TypeAnalyser_<DummyAccessor>(DummyAccessor(), e.z);
  }

  struct CrdAccessor {
    template <class T>
    const vec3d& operator () (const T& a) const {
      return olx_ref::get(a).crd();
    }
    typedef vec3d return_type;
  };

  struct CAtomAccessor {
    template <class T>
    const TCAtom& operator () (const T& a) const {
      return olx_ref::get(a).CAtom();
    }
    typedef TCAtom return_type;
  };

};

typedef TTypeList<TSAtom> TSAtomList;
typedef TPtrList<TSAtom> TSAtomPList;
typedef TPtrList<const TSAtom> TSAtomCPList;

EndXlibNamespace()
#endif
