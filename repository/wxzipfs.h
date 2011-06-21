#ifndef __olx_wxzip_fs_H
#define __olx_wxzip_fs_H
#include "filesystem.h"

#ifdef __WXWIDGETS__
#include <wx/zipstrm.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include "fsext.h"

struct TZipEntry  {
  olxstr ZipName;
  olxstr EntryName;
};

class TZipWrapper  {
  wxZipInputStream *FInputStream;
  wxFile *wxfile;
  TSStrPObjList<olxstr,wxZipEntry*, false> FEntries;
  TSStrPObjList<olxstr,TMemoryBlock*, false> FMemoryBlocks;
  TActionQList Actions;
protected:
  TMemoryBlock* GetMemoryBlock(const olxstr &EM);
  olxstr zip_name;
  bool UseCache;
  bool Break;
public:
  static olxstr ZipUrlSignature;

  TZipWrapper(const olxstr &zipName, bool useCache);
  TZipWrapper(TEFile* zipName, bool useCache);
  
  TActionQueue& OnProgress;

  ~TZipWrapper();
  IDataInputStream* OpenEntry(const olxstr& EN);
  wxInputStream* OpenWxEntry(const olxstr& EN);
  bool ExtractAll(const olxstr& dest);
  inline size_t Count() const {  return FEntries.Count();  }
  inline const olxstr& Name(size_t i) const {  return FEntries.GetString(i);  }
  inline time_t Timestamp(size_t i) const {  return FEntries.GetObject(i)->GetDateTime().GetTicks();  } 
  inline size_t Size(size_t i) const {  return FEntries.GetObject(i)->GetSize();  } 
  inline bool FileExists(const olxstr& fn) const {  return FEntries[TEFile::UnixPath(fn)] != NULL;  }

  static bool IsValidFileName(const olxstr &FN);
  static bool IsZipFile(const olxstr &FN);
  static olxstr ExtractZipName(const olxstr &FN);
  static olxstr ExtractZipEntryName(const olxstr &FN);
  static bool SplitZipUrl(const olxstr &fullName, TZipEntry &ZE);
  static olxstr ComposeFileName(const olxstr &ZipFileNameA, const olxstr &FNA);
  void DoBreak()  {  Break = true;  }
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TwxZipFileSystem: public AFileSystem  {
  TZipWrapper zip;
protected:
  // proxying functions
  virtual bool Enter(const IEObject *Sender, const IEObject *Data) {  
    if( Data != NULL && EsdlInstanceOf(*Data, TOnProgress) )  {
      OnProgress.Enter(this, Data);
      return true;
    }
    else
      return AFileSystem::Enter(Sender, Data);
  }
  virtual bool Exit(const IEObject *Sender, const IEObject *Data=NULL)  {  
    if( Data != NULL && EsdlInstanceOf(*Data, TOnProgress) )  {
      OnProgress.Exit(this, Data);
      return true;
    }
    else
      return AFileSystem::Exit(Sender, Data);
  }
  virtual bool Execute(const IEObject *Sender, const IEObject *Data=NULL) {  
    if( Data != NULL && EsdlInstanceOf(*Data, TOnProgress) )  {
      OnProgress.Execute(this, Data);
      return true;
    }
    else
      return AFileSystem::Execute(Sender, Data);
  }
  virtual bool _DoDelFile(const olxstr& f) {  return false;  }
  virtual bool _DoDelDir(const olxstr& f)  {  return false;  }
  virtual bool _DoNewDir(const olxstr& f)  {  return false;  }
  virtual bool _DoAdoptFile(const TFSItem& Source) {  return false;  }
  virtual bool _DoesExist(const olxstr& df, bool)  {  return zip.FileExists(df);  }
  virtual IInputStream* _DoOpenFile(const olxstr& src);
  virtual bool _DoAdoptStream(IInputStream& file, const olxstr& name) {  return false;  }
public:
  TwxZipFileSystem(const olxstr& filename, bool UseCache=false);
  TwxZipFileSystem(TEFile* file, bool UseCache);
  virtual ~TwxZipFileSystem() {}

  bool ExtractAll(const olxstr& dest);

  virtual void DoBreak()  {
    AFileSystem::DoBreak();
    zip.DoBreak();
  }
  virtual bool DelFile(const olxstr& FN)     {  throw TNotImplementedException(__OlxSourceInfo);    }
  virtual bool DelDir(const olxstr& DN)      {  throw TNotImplementedException(__OlxSourceInfo);     }
  virtual bool AdoptFile(const TFSItem& Source){  throw TNotImplementedException(__OlxSourceInfo);  }
  virtual bool AdoptStream(IInputStream& in, const olxstr& as){  throw TNotImplementedException(__OlxSourceInfo);  }
  virtual bool NewDir(const olxstr& DN)      {  throw TNotImplementedException(__OlxSourceInfo);     }
  virtual bool ChangeDir(const olxstr& DN)   {  throw TNotImplementedException(__OlxSourceInfo);  }
};

class TwxInputStreamWrapper : public IInputStream {
  wxInputStream& is;
public:
  TwxInputStreamWrapper(wxInputStream& _is) : is(_is) {}
  virtual void Read(void* data, size_t sz)  {
    is.Read(data, sz);
  }
  virtual void SetPosition(uint64_t i) {  is.SeekI(OlxIStream::CheckSize<off_t>(i));  }
  virtual uint64_t GetPosition() const {  return is.TellI();  }
  virtual uint64_t GetSize() const {  return is.GetSize();  }
};

class TwxOutputStreamWrapper : public IOutputStream {
  wxOutputStream& os;
public:
  TwxOutputStreamWrapper(wxOutputStream& _os) : os(_os) {}
  virtual size_t Write(const void* data, size_t sz)  {
    return os.Write(data, sz).LastWrite();
  }
  virtual void SetPosition(uint64_t i) {  os.SeekO(OlxIStream::CheckSize<off_t>(i));  }
  virtual uint64_t GetPosition() const {  return os.TellO();  }
  virtual uint64_t GetSize() const {  return os.GetSize();  }
};

#endif  // __WXWIDGETS__
#endif  
