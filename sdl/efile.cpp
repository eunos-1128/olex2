//---------------------------------------------------------------------------//
// (c) Oleg V. Dolomanov, 2004
//---------------------------------------------------------------------------//

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

//#include "smart/ostring.h"
#include "ebase.h"
#include <string.h>
#include <sys/stat.h>
#include "efile.h"

#ifdef __WIN32__

  #include <malloc.h>
  #include <io.h>
  #include <direct.h>

  #ifdef _MSC_VER
    #define chmod _chmod
    #define chdir _chdir
    #define access _access
    #define getcwd _getcwd
    #define makedir _mkdir
    #include <sys/utime.h>
    #define UTIMBUF _utimbuf
  #else
    #include <dirent.h>  // for gcc
    #define makedir mkdir
    #include <utime.h>
    #define UTIMBUF utimbuf
  #endif

  #define UTIME _utime
  #define unlink _unlink
  #define rmdir _rmdir
  //this is only for UNC file names under windows
  #include <windows.h>
  #include <mapiutil.h>

  #define OLX_PATH_DEL '\\'
  #define OLX_ENVI_PATH_DEL ';'
  #define OLX_OS_PATH(A)  TEFile::WinPath( (A) )

#else
  #include <unistd.h>
  #include <stdlib.h>
  #include <dirent.h>
  #include <utime.h>

  #define makedir(a) mkdir((a), 0755)
  #define UTIME utime

  #define OLX_PATH_DEL '/'
  #define OLX_ENVI_PATH_DEL ':'
  #define OLX_OS_PATH(A)  TEFile::UnixPath( (A) )
  #define UTIMBUF utimbuf
#endif


#include "exception.h"
#include "estrlist.h"

#include "library.h"
#include "bapp.h"

#ifndef MAX_PATH
  #define MAX_PATH 1024
#endif

UseEsdlNamespace()

//----------------------------------------------------------------------------//
// TFileNameMask function bodies
//----------------------------------------------------------------------------//
TEFile::TFileNameMask::TFileNameMask(const olxstr& msk )  {
  mask = msk;
  toks.Strtok( mask, '*');
  if( !mask.IsEmpty() )  {
    toksStart = (mask[0] != '*') ? 1 : 0;
    toksEnd = toks.Count() - ((mask[mask.Length()-1] != '*') ? 1 : 0);
  }
  else  {
    toksStart = 0;
    toksEnd = toks.Count();
  }
}
//..............................................................................
bool TEFile::TFileNameMask::DoesMatch(const olxstr& str)  const {
  if( mask.IsEmpty() && !str.IsEmpty() )  return false;
  // this will work for '*' mask
  if( toks.Count() == 0 )  return true;
  // need to check if the mask starts from a '*' or ends with it

  int off = 0, start = 0, end = str.Length();
  if( mask[0] != '*' )  {
    olxstr& tmp = toks.String(0);
    if( tmp.Length() > str.Length() )  return false;
    for( int i=0; i < tmp.Length(); i++ )
     if( !(tmp[i] == '?' || tmp[i] == str[i]) )  return false;
    start = tmp.Length();
    if( toks.Count() == 1 && mask[ mask.Length()-1] != '*' )  return true;
  }
  if( mask[ mask.Length()-1] != '*' && toks.Count() > (mask[0]!='*' ? 1 : 0) )  {
    olxstr& tmp = toks.String( toks.Count()-1 );
    if( tmp.Length() > (str.Length()-start) )  return false;
    for( int i=0; i < tmp.Length(); i++ )
     if( !(tmp[i] == '?' || tmp[i] == str[str.Length()-tmp.Length() + i]) )  return false;
    end = str.Length() - tmp.Length();

    if( toks.Count() == 1 )  return true;
  }

  for( int i=toksStart; i < toksEnd; i++ )  {
    olxstr& tmp = toks.String(i);
    bool found = false;
    for( int j=start; j < end; j++ )  {
      if( (str.Length() - j) < tmp.Length() )  return false;
      if( tmp[off] == '?' || str[j] == tmp[off] )  {
        while( tmp[off] == '?' || tmp[off] == str[j+off] )  {
          off++;
          if( off == tmp.Length() )  break;
        }
        start = j+off;
        if( off == tmp.Length() )  {  // found the mask string
          found = true;
          off = 0;
          break;
        }
        off = 0;
      }
    }
    if( !found )  return false;
  }
  return true;
}



//----------------------------------------------------------------------------//
// TEFile function bodies
//----------------------------------------------------------------------------//
TEFile::TEFile()  {
  FHandle = NULL;
}
//..............................................................................
TEFile::TEFile(const olxstr &F, const olxstr &Attribs)  {
  FHandle = NULL;
  Open(F, Attribs);
}
//..............................................................................
TEFile::TEFile(const olxstr& F, short Attribs)  {
  throw TNotImplementedException(__OlxSourceInfo);
/*
  olxstr attr;
  if( (Attribs & fofRead) != 0 )  {
    attr << 'r';
    if( (Attribs & fofWrite) != 0 )
      attr << 'w';
    if( (Attribs & fofAppend) != 0 )
      attr << 'a';
    if( (Attribs & fofCreate) != 0 )
      attr << '+';
  }
  else if( (Attribs & fofWrite) != 0 )  {
  }
*/
}
//..............................................................................
TEFile::~TEFile()  {  Close();  }
//..............................................................................
bool TEFile::Open(const olxstr& F, const olxstr& Attribs)  {
  Close();
  FName = OLX_OS_PATH(F);
  FHandle = fopen( FName.c_str(), Attribs.c_str());
  if( FHandle == NULL )  {
    olxstr fn = FName;
    FName = EmptyString;
    throw TFileExceptionBase(__OlxSourceInfo, F, olxstr("NULL handle for '") << F << '\'');
  }
  return true;
}
//..............................................................................
void TEFile::Close()  {
  if( FHandle != NULL )  {
    if( fclose(FHandle) != 0 )
      throw TFileExceptionBase(__OlxSourceInfo, FName, "fclose failed");
  }
  FHandle = NULL;
}
//..............................................................................
bool TEFile::Delete()  {
  if( FHandle == NULL )  return false;
  Close();
  return TEFile::DelFile(FName);
}
//..............................................................................
void TEFile::CheckHandle() const  {
  if( FHandle == NULL )
    throw TFileExceptionBase(__OlxSourceInfo, EmptyString, "Invalid file handle");
}
//..............................................................................
void TEFile::Read(void *Bf, size_t count)  {
  CheckHandle();
  if( count == 0 )  return;
  int res = fread(Bf, count, 1, FHandle);
  if( res != 1 )
    throw TFileExceptionBase(__OlxSourceInfo, FName, "fread failed" );
}
//..............................................................................
void TEFile::SetPosition(size_t p)  {
  CheckHandle();
  if( fseek(FHandle, p, SEEK_SET) != 0 )  
    throw TFileExceptionBase(__OlxSourceInfo, FName, "fseek failed" );
}
//..............................................................................
size_t TEFile::GetPosition() const  {
  CheckHandle();
  long v = ftell(FHandle);
  if( v == -1 )
    throw TFileExceptionBase(__OlxSourceInfo, FName, "ftell failed" );
  return v;
}
//..............................................................................
long TEFile::Length() const  {
  CheckHandle();
  size_t currentPos = GetPosition();
  if( fseek( FHandle, 0, SEEK_END) )  
    throw TFileExceptionBase(__OlxSourceInfo, FName, "fseek failed");
  long length = ftell( FHandle );
  if( length == -1 )
    throw TFileExceptionBase(__OlxSourceInfo, FName, "ftell failed" );
  fseek( FHandle, currentPos, SEEK_SET);
  return length;
}
//..............................................................................
void TEFile::Flush()  {
  CheckHandle();
  fflush(FHandle);
}
//..............................................................................
size_t TEFile::Write(const void *Bf, size_t count)  {
  CheckHandle();
  if( count == 0 )  return count;
  size_t res = fwrite(Bf, count, 1, FHandle);
  if( res == 0 )
    throw TFileExceptionBase(__OlxSourceInfo, FName, "fwrite failed" );
  return res;
}
//..............................................................................
void TEFile::Seek( long Position, const int From)  {
  CheckHandle();
  if( fseek(FHandle, Position, From) != 0 )
    throw TFileExceptionBase(__OlxSourceInfo, FName, "fseek failed");
}
//..............................................................................
bool TEFile::FileExists(const olxstr& F)  {
  return (access( OLX_OS_PATH(F).c_str(), 0)  == -1 ) ? false : true;
}
//..............................................................................
olxstr TEFile::ExtractFilePath(const olxstr &F)  {
  olxstr fn = OLX_OS_PATH(F);
  if( TEFile::IsAbsolutePath( F ) )  {
    int i = fn.LastIndexOf( OLX_PATH_DEL );
    if( i > 0 ) return fn.SubStringTo(i+1);
    return EmptyString;
  }
  return EmptyString;
}
//..............................................................................
olxstr TEFile::ParentDir(const olxstr& F) {
  if( F.IsEmpty() )  return F;
  int start = F[F.Length()-1] == OLX_PATH_DEL ? F.Length()-2 : F.Length()-1;
  olxstr fn = OLX_OS_PATH(F);
  int i = fn.LastIndexOf(OLX_PATH_DEL, start);
  if( i > 0 ) return fn.SubStringTo(i+1);
  return EmptyString;
}
//..............................................................................
olxstr TEFile::ExtractFileExt(const olxstr& F)  {
  int i=F.LastIndexOf('.');
  if( i > 0 )  return F.SubStringFrom(i+1);
  return EmptyString;
}
//..............................................................................
olxstr TEFile::ExtractFileName(const olxstr& F)  {
  olxstr fn = OLX_OS_PATH(F);
  int i=fn.LastIndexOf( OLX_PATH_DEL );
  if( i > 0 )  return fn.SubStringFrom(i+1);
  return F;
}
//..............................................................................
olxstr TEFile::ExtractFileDrive(const olxstr& F)  {
#ifdef __WIN32__
  if( F.Length() < 2 )  return EmptyString;
  if( F[1] != ':' )  return EmptyString;
  return F.SubString(0, 2);
#else
  return F;
#endif
}
//..............................................................................
olxstr TEFile::ChangeFileExt(const olxstr &F, const olxstr &Ext)  {
  if( F.IsEmpty() )  return F;
  olxstr N(F);

  int i=N.LastIndexOf('.');
  if( i > 0 )  N.SetLength(i);
  else  {
    if( N[N.Length()-1] == '.' )
      N.SetLength(N.Length()-1);
  }
  if( Ext.Length() )  {
    if( Ext[0] != '.' )  N << '.';
    N << Ext;
  }
  return N;
}
//..............................................................................
bool TEFile::DelFile(const olxstr& F)  {
  if( !TEFile::FileExists(F) )  return true;
  olxstr fn = OLX_OS_PATH(F);
  if( !chmod(fn.c_str(), S_IWRITE) )
    return (unlink(fn.c_str()) == -1) ? false: true;
  return false;
}
//..............................................................................
bool TEFile::DelDir(const olxstr& F)  {
  if( !TEFile::FileExists(F) )  return true;
  olxstr fn = OLX_OS_PATH(F);
  return (rmdir(fn.c_str()) == -1) ?  false : true;
}
//..............................................................................
#ifdef __BORLANDC__
bool TEFile::ListCurrentDirEx(TFileList &Out, const olxstr &Mask, const unsigned short sF)  {
  olxstr Tmp;
  struct ffblk fdata;
  struct stat the_stat;

  int done, flags = 0;
  unsigned short attrib;
  TStrList L;
  if( (sF & sefDir) != 0 )       flags |= FA_DIREC;
  if( (sF & sefReadOnly) != 0 )  flags |= FA_RDONLY;
  if( (sF & sefSystem) != 0 )    flags |= FA_SYSTEM;
  if( (sF & sefHidden) != 0 )    flags |= FA_HIDDEN;

  L.Strtok(Mask, ';');
  for( int i=0; i < L.Count(); i++ )  {
    done = findfirst(L.String(i).c_str(), &fdata, flags);
    while( !done )  {
      if( (sF & sefDir) != 0 && (sF & sefRelDir) == 0 &&
          (fdata.ff_attrib & FA_DIREC) != 0)  {
        if( strcmp(fdata.ff_name, ".") == 0 || strcmp(fdata.ff_name, "..") == 0)  {
          done = findnext(&fdata);
          continue;
        }
      }
      stat(fdata.ff_name, &the_stat);
      TFileListItem& li = Out.AddNew();
      li.SetName( fdata.ff_name );
      li.SetSize( fdata.ff_fsize );
      li.SetCreationTime( the_stat.st_ctime );
      li.SetModificationTime( the_stat.st_mtime );
      li.SetLastAccessTime( the_stat.st_atime );
      attrib = 0;
      if( (fdata.ff_attrib & FA_DIREC) != 0 )  attrib |= sefDir;
      else                                      attrib |= sefFile;
      if( (fdata.ff_attrib & FA_RDONLY) != 0 ) attrib |= sefReadOnly;
      if( (fdata.ff_attrib & FA_SYSTEM) != 0 ) attrib |= sefSystem;
      if( (fdata.ff_attrib & FA_HIDDEN) != 0 ) attrib |= sefHidden;
      li.SetAttributes( attrib );
      done = findnext(&fdata);
    }
  }
  findclose( &fdata );
  return true;
}
bool TEFile::ListCurrentDir(TStrList& Out, const olxstr &Mask, const unsigned short sF)  {
  olxstr Tmp;
  struct ffblk fdata;
  int done, flags = 0;
  TStrList L;
  if( (sF & sefDir) != 0 )       flags |= FA_DIREC;
  if( (sF & sefReadOnly) != 0 )  flags |= FA_RDONLY;
  if( (sF & sefSystem) != 0 )    flags |= FA_SYSTEM;
  if( (sF & sefHidden) != 0 )    flags |= FA_HIDDEN;

  L.Strtok(Mask, ';');
  for( int i=0; i < L.Count(); i++ )  {
    done = findfirst(L.String(i).c_str(), &fdata, flags);
    while( !done )  {
      if( (sF & sefDir) != 0 && (sF & sefRelDir) == 0 &&
          (fdata.ff_attrib & FA_DIREC) != 0)  {
        if( strcmp(fdata.ff_name, ".") == 0 || strcmp(fdata.ff_name, "..") == 0)  {
          done = findnext(&fdata);
          continue;
        }
      }
      Out.Add( fdata.ff_name );
      done = findnext(&fdata);
    }
  }
  findclose( &fdata );
  return true;
}
#elif _MSC_VER
bool TEFile::ListCurrentDirEx(TFileList &Out, const olxstr &Mask, const unsigned short sF)  {
  olxstr Tmp;
  struct _finddata_t c_file;
  c_file.attrib = 0;
  unsigned short attrib;

  if( (sF & sefDir) != 0 )       c_file.attrib |= _A_SUBDIR;
  if( (sF & sefReadOnly) != 0 )  c_file.attrib |= _A_RDONLY;
  if( (sF & sefSystem) != 0 )    c_file.attrib |= _A_SYSTEM;
  if( (sF & sefHidden) != 0 )    c_file.attrib |= _A_HIDDEN;

  intptr_t hFile;
  TStrList L(Mask, ';');
  for( int i=0; i < L.Count(); i++ )  {
    hFile = _findfirst( L[i].c_str(), &c_file);
    if( hFile == -1 )  continue;
    do  {
      if( (sF & sefDir) != 0 && (sF & sefRelDir) == 0 &&
          (c_file.attrib & _A_SUBDIR) != 0)  {
        if( strcmp(c_file.name, ".") == 0 || strcmp(c_file.name, "..") == 0)  {
          if( _findnext(hFile, &c_file) != 0 )  break;
          continue;
        }
      }
      TFileListItem& li = Out.AddNew();
      li.SetName( c_file.name );
      li.SetSize( c_file.size );
      li.SetCreationTime( c_file.time_create );
      li.SetLastAccessTime( c_file.time_access );
      li.SetModificationTime( c_file.time_write );
      attrib = 0;
      if( (c_file.attrib & _A_SUBDIR) != 0 ) attrib |= sefDir;
      else                                   attrib |= sefFile;
      if( (c_file.attrib & _A_RDONLY) != 0 ) attrib |= sefReadOnly;
      if( (c_file.attrib & _A_SYSTEM) != 0 ) attrib |= sefSystem;
      if( (c_file.attrib & _A_HIDDEN) != 0 ) attrib |= sefHidden;
      li.SetAttributes( attrib );
    }
    while( _findnext(hFile, &c_file) == 0 );
  }
  _findclose(hFile);
  return true;
}
bool TEFile::ListCurrentDir(TStrList &Out, const olxstr &Mask, const unsigned short sF)  {
  olxstr Tmp;
  struct _finddata_t c_file;
  c_file.attrib = 0;

  if( (sF & sefDir) != 0 )       c_file.attrib |= _A_SUBDIR;
  if( (sF & sefReadOnly) != 0 )  c_file.attrib |= _A_RDONLY;
  if( (sF & sefSystem) != 0 )    c_file.attrib |= _A_SYSTEM;
  if( (sF & sefHidden) != 0 )    c_file.attrib |= _A_HIDDEN;

  intptr_t hFile;
  TStrList L(Mask, ';');
  for( int i=0; i < L.Count(); i++ )  {
    hFile = _findfirst( L[i].c_str(), &c_file);
    if( hFile == -1 )  continue;
    do  {
      if( (sF & sefDir) != 0 && (sF & sefRelDir) == 0 &&
          (c_file.attrib & _A_SUBDIR) != 0)  {
        if( strcmp(c_file.name, ".") == 0 || strcmp(c_file.name, "..") == 0)  {
          if( _findnext(hFile, &c_file) != 0 )  break;
          continue;
        }
      }
      Out.Add( c_file.name );
    }
    while( _findnext(hFile, &c_file) == 0 );
  }
  _findclose(hFile);
  return true;
}
#else
//..............................................................................
bool TEFile::ListCurrentDirEx(TFileList &Out, const olxstr &Mask, const unsigned short sF)  {
  DIR *d = opendir( TEFile::CurrentDir().c_str() );
  if( d == NULL ) return false;
  TStrList ml(Mask, ';');
  TTypeList<AnAssociation2<TEFile::TFileNameMask*, TEFile::TFileNameMask*> > masks;
  olxstr tmp, fn;
  for(int i=0; i < ml.Count(); i++ )  {
    olxstr& t = ml.String(i);
    tmp = TEFile::ExtractFileExt( t );
    masks.AddNew( new TEFile::TFileNameMask(t.SubStringTo(t.Length() - tmp.Length() - (tmp.Length()!=0 ? 1 : 0))),
                  new TEFile::TFileNameMask(tmp) );
  }
  int access = 0, faccess;
  unsigned short attrib;
  if( (sF & sefReadOnly) != 0 )  access |= S_IRUSR;
  if( (sF & sefWriteOnly) != 0 ) access |= S_IWUSR;
  if( (sF & sefExecute) != 0 )   access |= S_IXUSR;

  struct stat the_stat;
  struct dirent* de;
  while((de = readdir(d)) != NULL) {
    stat(de->d_name, &the_stat);
    if( sF != sefAll )  {
      faccess = 0;
      if( (the_stat.st_mode & S_IRUSR) != 0 )  faccess |= S_IRUSR;
      if( (the_stat.st_mode & S_IWUSR) != 0 )  faccess |= S_IWUSR;
      if( (the_stat.st_mode & S_IXUSR) != 0 )  faccess |= S_IXUSR;
      if( (faccess & access) != access )  continue;

      if( (sF & sefDir) != 0 )  {
        if( S_ISDIR(the_stat.st_mode) )  {
          if( (sF & sefRelDir) == 0 )
            if( strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 )
              continue;
        }
        else if( (sF & sefFile) == 0 )  continue;
      }
      else if( (sF & sefFile) == 0 || S_ISDIR(the_stat.st_mode) )  continue;
    }
    fn = de->d_name;
    for( int i=0; i < masks.Count(); i++ )  {
      tmp = TEFile::ExtractFileExt( fn );
      if( masks[i].GetB()->DoesMatch( tmp ) &&
          masks[i].GetA()->DoesMatch( fn.SubStringTo(fn.Length() - tmp.Length() - (tmp.Length()!=0 ? 1 : 0)) ) )
      {
        TFileListItem& li = Out.AddNew();
        li.SetName( de->d_name );
        li.SetSize( the_stat.st_size );
        li.SetCreationTime( the_stat.st_ctime );
        li.SetModificationTime( the_stat.st_mtime );
        li.SetLastAccessTime( the_stat.st_atime );
        attrib = 0;
        if( S_ISDIR(the_stat.st_mode) )           attrib |= sefDir;
        else                                      attrib |= sefFile;
        if( (the_stat.st_mode & S_IRUSR) != 0 )  attrib |= sefReadOnly;
        if( (the_stat.st_mode & S_IWUSR) != 0 )  attrib |= sefWriteOnly;
        if( (the_stat.st_mode & S_IXUSR) != 0 )  attrib |= sefExecute;
        li.SetAttributes( attrib );
        break;
      }
    }
  }
  for( int i=0; i < masks.Count(); i++ )  {
    delete masks[i].GetA();
    delete masks[i].GetB();
  }
  return closedir(d) == 0;
}
//..............................................................................
bool TEFile::ListCurrentDir(TStrList &Out, const olxstr &Mask, const unsigned short sF)  {
  DIR *d = opendir( TEFile::CurrentDir().c_str() );
  if( d == NULL ) return false;
  TStrList ml(Mask, ';');
  TTypeList<AnAssociation2<TEFile::TFileNameMask*, TEFile::TFileNameMask*> > masks;
  olxstr tmp, fn;
  for(int i=0; i < ml.Count(); i++ )  {
    olxstr& t = ml.String(i);
    tmp = TEFile::ExtractFileExt( t );
    masks.AddNew( new TEFile::TFileNameMask(t.SubStringTo(t.Length() - tmp.Length() - (tmp.Length()!=0 ? 1 : 0))),
                  new TEFile::TFileNameMask(tmp) );
  }
  int access = 0, faccess;

  if( (sF & sefReadOnly) != 0 )  access |= S_IRUSR;
  if( (sF & sefWriteOnly) != 0 ) access |= S_IWUSR;
  if( (sF & sefExecute) != 0 )   access |= S_IXUSR;

  struct stat the_stat;
  struct dirent* de;
  while((de = readdir(d)) != NULL) {
    if( sF != sefAll )  {
      stat(de->d_name, &the_stat);
      faccess = 0;
      if( (the_stat.st_mode & S_IRUSR) != 0 )  faccess |= S_IRUSR;
      if( (the_stat.st_mode & S_IWUSR) != 0 )  faccess |= S_IWUSR;
      if( (the_stat.st_mode & S_IXUSR) != 0 )  faccess |= S_IXUSR;
      if( (faccess & access) != access )  continue;

      if( (sF & sefDir) != 0 )  {
        if( S_ISDIR(the_stat.st_mode) )  {
          if( (sF & sefRelDir) == 0 )
            if( strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 )
              continue;
        }
        else if( (sF & sefFile) == 0 )  continue;
      }
      else if( (sF & sefFile) == 0 || S_ISDIR(the_stat.st_mode) )  continue;
    }
    fn = de->d_name;
    for( int i=0; i < masks.Count(); i++ )  {
      tmp = TEFile::ExtractFileExt( fn );
      if( masks[i].GetB()->DoesMatch( tmp ) &&
          masks[i].GetA()->DoesMatch( fn.SubStringTo(fn.Length() - tmp.Length() - (tmp.Length()!=0 ? 1 : 0)) ) )
      {
        Out.Add(de->d_name);
        break;
      }
    }
  }
  for( int i=0; i < masks.Count(); i++ )  {
    delete masks[i].GetA();
    delete masks[i].GetB();
  }
  return closedir(d) == 0;
}
#endif
//..............................................................................
bool TEFile::SetFileTimes(const olxstr& fileName, long AccTime, long ModTime)  {
  struct UTIMBUF tb;
  tb.actime = AccTime;
  tb.modtime = ModTime;
  return UTIME(fileName.c_str(), &tb) == 0 ? true : false;
}
//..............................................................................
// thanx to Luc - I have completely forgotten about stat!
time_t TEFile::FileAge(const olxstr& fileName)  {
  struct stat the_stat;
  if( stat(OLX_OS_PATH(fileName).c_str(), &the_stat) != 0 )
    throw TInvalidArgumentException(__OlxSourceInfo, olxstr("Invalid file '") << fileName << '\'');
#ifdef __BORLANDC__
  return the_stat.st_mtime;
#elif _MSC_VER
  return the_stat.st_mtime;
#elif __GNUC__
  return the_stat.st_mtime;
#else
  struct timespec& t = the_stat.st_mtimespec;
  return t.tv_nsec + t.tv_nsec*1e-9; // number of seconds since last modification
#endif
}
//..............................................................................
long TEFile::FileLength(const olxstr& fileName)  {
  struct stat the_stat;
  stat(OLX_OS_PATH(fileName).c_str(), &the_stat);
  return the_stat.st_size;
}
//..............................................................................
bool TEFile::ChangeDir(const olxstr& To)  {
  if( To.Length() == 0 )  return false;
  return ( chdir(OLX_OS_PATH(To).c_str()) == -1 ) ?  false : true;
}
//..............................................................................
olxstr TEFile::CurrentDir()  {
  char *Dp = getcwd(NULL, MAX_PATH);
  olxstr Dir = Dp;
  free(Dp);
  return Dir;
}
//..............................................................................
bool TEFile::MakeDirs(const olxstr& Name)  {
  TStrList toks(OLX_OS_PATH(Name), OLX_PATH_DEL);
  olxstr toCreate;
  toCreate.SetCapacity( Name.Length() + 5 );
  for( int i=0; i < toks.Count(); i++ )  {
    toCreate << toks.String(i) << OLX_PATH_DEL;
    if( !FileExists( toCreate ) )
      if( makedir( toCreate.c_str() ) == -1 )
        return false;
  }
  return true;
}
//..............................................................................
bool TEFile::MakeDir(const olxstr& Name)  {
  return ( makedir(OLX_OS_PATH(Name).c_str()) == -1 ) ? false : true;
}
//..............................................................................
olxstr TEFile::OSPath(const olxstr &F)  {
  return OLX_OS_PATH(F);
}
//..............................................................................
olxstr TEFile::WinPath(const olxstr &F)  {
  olxstr T(F);
  T.Replace('/', '\\');
  return T;
}
//..............................................................................
olxstr TEFile::UnixPath(const olxstr& F) {
  olxstr T(F);
  T.Replace('\\', '/');
  return T;
}
//..............................................................................
olxstr& TEFile::WinPathI( olxstr& F )  {
  F.Replace('/', '\\');
  return F;
}
//..............................................................................
olxstr& TEFile::UnixPathI( olxstr& F )  {
  F.Replace('\\', '/');
  return F;
}
//..............................................................................
olxstr TEFile::AddTrailingBackslash( const olxstr& Path )  {
  if( Path.IsEmpty() )  return Path;
  olxstr T( Path );
  if( T[T.Length()-1] != OLX_PATH_DEL )  T << OLX_PATH_DEL;
  return T;
}
//..............................................................................
olxstr& TEFile::AddTrailingBackslashI(olxstr &Path)  {
  if( Path.IsEmpty() )  return Path;
  if( Path[Path.Length()-1] != OLX_PATH_DEL )  Path << OLX_PATH_DEL;
  return Path;
}
//..............................................................................
olxstr TEFile::RemoveTrailingBackslash(const olxstr &Path)  {
  if( !Path.Length() )  return Path;
  return (Path[Path.Length()-1] == OLX_PATH_DEL ) ? Path.SubStringTo(Path.Length()-1) : Path;
}
//..............................................................................
olxstr& TEFile::RemoveTrailingBackslashI(olxstr &Path)  {
  if( !Path.Length() )  return Path;
  if( Path[Path.Length()-1] == OLX_PATH_DEL )
    Path.SetLength(Path.Length()-1);
  return Path;
}
//..............................................................................
bool TEFile::IsAbsolutePath(const olxstr& Path)  {
  if( Path.Length() < 2 )  return false;
#ifdef __WIN32__
  if( Path[1] == ':' )  return true;
  if( Path[0] == '\\' && Path[1] == '\\' )  return true;
  return false;
#else
  return (Path[0] == '/') ? true : false;
#endif
}
//..............................................................................
olxstr TEFile::UNCFileName(const olxstr &LocalFN)  {
#ifdef __WIN32__  //this function does not work on winxp anymore ...
//  char buffer[512];
//  memset( buffer, 0, 512);
//  if(  ScUNCFromLocalPath(LocalFN.c_str(), buffer, 512) == S_OK )
//    return olxstr(buffer);
  return LocalFN;
#else
  return LocalFN;
#endif
}
//..............................................................................
void TEFile::CheckFileExists(const olxstr& location, const olxstr& fileName)  {
  if( !TEFile::FileExists(fileName) )
    throw TFileDoesNotExistException(location, fileName);
}
//..............................................................................
void TEFile::Copy(const olxstr& From, const olxstr& To, bool overwrite )  {
  if( TEFile::FileExists(To) && !overwrite )  return;
  // need to check that the files are not the same though...
  TEFile in( From, "rb" );
  TEFile out( To, "w+b" );
  out << in;
}
//..............................................................................
olxstr TEFile::AbsolutePathTo(const olxstr &Path, const olxstr &relPath ) {
  TStrList dirToks(OLX_OS_PATH( Path ), OLX_PATH_DEL),
              relPathToks(OLX_OS_PATH( relPath ), OLX_PATH_DEL);
  for( int i=0; i < relPathToks.Count(); i++ )  {
    if( relPathToks.String(i) == ".." )
      dirToks.Delete( dirToks.Count()-1 );
    else if( relPathToks.String(i) == "." )
      ;
    else
      dirToks.Add( relPathToks.String(i) );
  }
  olxstr res = dirToks.Text(OLX_PATH_DEL);
//  if( !TEFile::FileExists( res ) )
//    throw TFileDoesNotExistException(__OlxSourceInfo, res);
  return res;
}
//..............................................................................
olxstr TEFile::Which(const olxstr& filename)  {
  if( TEFile::IsAbsolutePath(filename) )
    return filename;
  olxstr fn = TEFile::CurrentDir();
  TEFile::AddTrailingBackslashI(fn) << filename;
  // check current folder
  if( TEFile::FileExists(fn) )  return fn;
  // check program folder
  fn = TBasicApp::GetInstance()->BaseDir();
  fn << filename;
  if( TEFile::FileExists(fn) )  return fn;
  // check path then ...
  char* path = getenv("PATH");
  if( path == NULL )  return EmptyString;
  TStrList toks(path, OLX_ENVI_PATH_DEL);
  for( int i=0; i < toks.Count(); i++ )  {
    TEFile::AddTrailingBackslashI(toks.String(i)) << filename;
    if( TEFile::FileExists(toks.String(i)) )
      return toks.String(i);
  }
  return EmptyString;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////EXTERNAL LIBRRAY FUNCTIONS//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void FileExists(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::FileExists( Params.String(0) ) );
}

void FileName(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::ExtractFileName( Params.String(0) ) );
}

void FilePath(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::ExtractFilePath( Params.String(0) ) );
}

void FileDrive(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::ExtractFileDrive( Params.String(0) ) );
}

void FileExt(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::ExtractFileExt( Params.String(0) ) );
}

void ChangeFileExt(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::ChangeFileExt( Params.String(0), Params.String(1) ) );
}

void Copy(const TStrObjList& Params, TMacroError& E)  {
  TEFile::Copy( Params.String(0), Params.String(1) );
  E.SetRetVal( Params.String(1) );
}

void Delete(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::DelFile(Params.String(0)) );
}

void CurDir(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::CurrentDir() );
}

void ChDir(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::ChangeDir(Params.String(0)) );
}

void MkDir(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::MakeDir(Params.String(0)) );
}

void OSPath(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::OSPath(Params.String(0)) );
}

void Which(const TStrObjList& Params, TMacroError& E)  {
  E.SetRetVal( TEFile::Which(Params.String(0)) );
}

void Age(const TStrObjList& Params, TMacroError& E)  {
  TEFile::CheckFileExists(__OlxSourceInfo, Params.String(0));
  time_t v = TEFile::FileAge( Params.String(0) );
  if( Params.Count() == 1 )
    E.SetRetVal( TETime::FormatDateTime(v) );
  else
    E.SetRetVal( TETime::FormatDateTime(Params.String(1), v) );
}

void ListDirForGUI(const TStrObjList& Params, TMacroError& E)  {
  TEFile::CheckFileExists(__OlxSourceInfo, Params.String(0));
  olxstr cd( TEFile::CurrentDir() );
  olxstr dn( Params.String(0) );
  TEFile::AddTrailingBackslashI(dn);
  TEFile::ChangeDir( Params.String(0) );
  short attrib = sefFile;
  if( Params.Count() == 3 )  {
    if( Params.String(2).Comparei("fd") == 0 )
      attrib |= sefDir;
    else if( Params.String(2)[0] == 'd' )
      attrib = sefDir;
  }
  TStrList output;
  olxstr tmp;
  TEFile::ListCurrentDir( output, Params.String(1), attrib );
  TEFile::ChangeDir( cd );
  output.QSort(false);
  for(int i=0; i < output.Count(); i++ )  {
   tmp = EmptyString;
    tmp <<  "<-" << dn << output.String(i);
    output.String(i) << tmp;
  }
  E.SetRetVal( output.Text(';') );
}

TLibrary*  TEFile::ExportLibrary(const olxstr& name)
{
  TLibrary* lib = new TLibrary(name.IsEmpty() ? olxstr("file") : name);
  lib->RegisterStaticFunction( new TStaticFunction( ::FileExists, "Exists", fpOne,
"Returns true if specified file exists") );
  lib->RegisterStaticFunction( new TStaticFunction( ::FileName, "GetName", fpOne,
"Returns name part of the full/partial file name") );
  lib->RegisterStaticFunction( new TStaticFunction( ::FilePath, "GetPath", fpOne,
"Returns path component of the full file name") );
  lib->RegisterStaticFunction( new TStaticFunction( ::FileDrive, "GetDrive", fpOne,
"Returns drive component of the full file name") );
  lib->RegisterStaticFunction( new TStaticFunction( ::FileExt, "GetExt", fpOne,
"Returns file extension") );
  lib->RegisterStaticFunction( new TStaticFunction( ::ChangeFileExt, "ChangeExt", fpTwo,
"Returns file name with changed extention") );
  lib->RegisterStaticFunction( new TStaticFunction( ::Copy, "Copy", fpTwo,
"Copies file provieded as first argument into the file provided as second argument") );
  lib->RegisterStaticFunction( new TStaticFunction( ::Delete, "Delete", fpOne,
"Deletes specified file") );
  lib->RegisterStaticFunction( new TStaticFunction( ::CurDir, "CurDir", fpNone,
"Returns current folder") );
  lib->RegisterStaticFunction( new TStaticFunction( ::ChDir, "ChDir", fpOne,
"Changes current folder to provieded folder") );
  lib->RegisterStaticFunction( new TStaticFunction( ::MkDir, "MkDir", fpOne,
"Creates specified folder") );
  lib->RegisterStaticFunction( new TStaticFunction( ::OSPath, "ospath", fpOne,
"Returns OS specific path for provided path") );
  lib->RegisterStaticFunction( new TStaticFunction( ::Which, "Which", fpOne,
"Tries to find a particular file looking at curent folder, PATH and program folder") );
  lib->RegisterStaticFunction( new TStaticFunction( ::Age, "Age", fpOne|fpTwo,
"Returns file age for provided file using formating string (if provided)") );
  lib->RegisterStaticFunction( new TStaticFunction( ::ListDirForGUI, "ListDirForGUI", fpTwo|fpThree,
"Returns a ready to use in gui list of files, matching provided mask(s) separated by semicolumn.\
 The third, optional argument [f,d,fd] specifies what shoul dbe included into the list") );
  return lib;
}

