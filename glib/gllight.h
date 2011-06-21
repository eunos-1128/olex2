#ifndef __olx_gl_light_H
#define __olx_gl_light_H
#include "glbase.h"
#include "datafile.h"
#include "gloption.h"
#include "library.h"
BeginGlNamespace()

class TGlLight : public IEObject  {
  TGlOption Ambient;
  TGlOption Diffuse;
  TGlOption Specular;
  TGlOption Position;
  TGlOption SpotDirection;
  short SpotCutoff;
  short SpotExponent;
  TGlOption Attenuation;
  bool Enabled;
  short Index;
  TActionQList actions;
public:
  TGlLight();
  virtual ~TGlLight() {}

  DefPropP(short, Index)
  DefPropBIsSet(Enabled)
  DefPropP(short, SpotCutoff)
  DefPropP(short, SpotExponent)
  DefPropC(TGlOption, Ambient)
  DefPropC(TGlOption, Diffuse)
  DefPropC(TGlOption, Specular)
  DefPropC(TGlOption, Position)
  DefPropC(TGlOption, SpotDirection)
  DefPropC(TGlOption, Attenuation)

  TGlLight& operator = (const TGlLight &S );
  void ToDataItem(TDataItem& Item) const;
  bool FromDataItem(const TDataItem& Item);

  TActionQueue& OnLibChange;

  void LibEnabled(const TStrObjList& Params, TMacroError& E);
  void LibSpotCutoff(const TStrObjList& Params, TMacroError& E);
  void LibSpotExponent(const TStrObjList& Params, TMacroError& E);
  void LibAmbient(const TStrObjList& Params, TMacroError& E);
  void LibDiffuse(const TStrObjList& Params, TMacroError& E);
  void LibSpecular(const TStrObjList& Params, TMacroError& E);
  void LibPosition(const TStrObjList& Params, TMacroError& E);
  void LibSpotDirection(const TStrObjList& Params, TMacroError& E);
  void LibAttenuation(const TStrObjList& Params, TMacroError& E);
  TLibrary* ExportLibrary(const olxstr& name);
};

EndGlNamespace()
#endif
