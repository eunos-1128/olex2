#ifndef htmlextH
#define htmlextH

#include "estrlist.h"
#include "paramlist.h"
#include "actions.h"
#include "../wininterface.h"
#include "wx/wxhtml.h"
#include "wx/dynarray.h"

#include "library.h"
#include "edict.h"
#include "../ctrls/olxctrlbase.h"

class THtmlSwitch;

class THtml: public wxHtmlWindow, public IEObject  {
private:
  bool Movable, PageLoadRequested, ShowTooltips;
  int LockPageLoad;
  olxstr PageRequested;
  wxWindow* InFocus;
  TActionQList Actions;
protected:
  olxstr  WebFolder, FileName, HomePage;   // the base of all web files
  olxstr NormalFont, FixedFont;
  void OnLinkClicked(const wxHtmlLinkInfo& link);
  wxHtmlOpeningStatus OnOpeningURL(wxHtmlURLType type, const wxString& url, wxString *redirect) const;

  void OnMouseDblClick(wxMouseEvent& event);
  void OnMouseDown(wxMouseEvent& event);
  void OnMouseUp(wxMouseEvent& event);
  void OnMouseMotion(wxMouseEvent& event);
  void OnCellMouseHover(wxHtmlCell *Cell, wxCoord x, wxCoord y);
  void OnChildFocus(wxChildFocusEvent& event);
  void DoHandleFocusEvent(AOlxCtrl* prev, AOlxCtrl* next);
  /* on GTK scrolling makes mess out of the controls so will try to "fix it" here*/
  void OnScroll(wxScrollEvent& evt);
  virtual void ScrollWindow(int dx, int dy, const wxRect* rect = NULL);

  // position of where the mous was down
  int MouseX, MouseY;
  bool MouseDown;

  THtmlSwitch* Root;
  olxdict<olxstr, AnAssociation3<AOlxCtrl*,wxWindow*,bool>, olxstrComparator<true> > Objects;
  TTypeList<AnAssociation2<AOlxCtrl*,wxWindow*> > Traversables;
  TSStrPObjList<olxstr,size_t,true> SwitchStates;
  olxstr FocusedControl;
  class TObjectsState  {
    TSStrPObjList<olxstr,TSStrStrList<olxstr,false>*, true> Objects;
    THtml& html;
  public:
    TObjectsState(THtml& htm) : html(htm) { }
    ~TObjectsState();
    TSStrStrList<olxstr,false>* FindProperties(const olxstr& cname) {
      const size_t ind = Objects.IndexOf(cname);
      return (ind == InvalidIndex) ? NULL : Objects.GetObject(ind);
    }
    TSStrStrList<olxstr,false>* DefineControl(const olxstr& name, const std::type_info& type);
    void SaveState();
    void RestoreState();
    void SaveToFile(const olxstr& fn);
    bool LoadFromFile(const olxstr& fn);
  };
  TObjectsState ObjectsState;
protected:
  size_t GetSwitchState(const olxstr& switchName);
  void ClearSwitchStates()  {  SwitchStates.Clear();  }
  // library
  DefMacro(ItemState)
    DefMacro(UpdateHtml)
    DefMacro(HtmlHome)
    DefMacro(HtmlReload)
    DefMacro(HtmlLoad)
    DefMacro(HtmlDump)
    DefMacro(Tooltips)
    DefMacro(SetFonts)
    DefMacro(SetBorders)
    DefMacro(DefineControl)
    DefMacro(Hide)

    DefFunc(GetValue)
    DefFunc(GetData)
    DefFunc(GetLabel)
    DefFunc(GetImage)
    DefFunc(GetState)
    DefFunc(GetItems)
    DefFunc(SetValue)
    DefFunc(SetData)
    DefFunc(SetLabel)
    DefFunc(SetImage)
    DefFunc(SetItems)
    DefFunc(SetState)
    DefFunc(SetFG)
    DefFunc(SetBG)
    DefFunc(GetFontName)
    DefFunc(GetBorders)
    DefFunc(SetFocus)
    DefFunc(EndModal)
    DefFunc(ShowModal)

    DefFunc(SaveData)
    DefFunc(LoadData)
    DefFunc(GetItemState)
    DefFunc(IsItem)
    DefFunc(IsPopup)

  olxstr GetObjectValue(const AOlxCtrl *Object);
  const olxstr& GetObjectData(const AOlxCtrl *Object);
  bool GetObjectState(const AOlxCtrl *Object);
  olxstr GetObjectImage(const AOlxCtrl *Object);
  olxstr GetObjectItems(const AOlxCtrl *Object);
  void SetObjectValue(AOlxCtrl *AOlxCtrl, const olxstr& Value);
  void SetObjectData(AOlxCtrl *AOlxCtrl, const olxstr& Data);
  void SetObjectState(AOlxCtrl *AOlxCtrl, bool State);
  bool SetObjectImage(AOlxCtrl *AOlxCtrl, const olxstr& src);
  bool SetObjectItems(AOlxCtrl *AOlxCtrl, const olxstr& src);
  void _FindNext(int from, int &dest, bool scroll) const;
  void _FindPrev(int from, int &dest, bool scroll) const;
  void GetTraversibleIndeces(int& current, int& another, bool forward) const;
  void DoNavigate(bool forward);
  static TLibrary* Library;
public:
  THtml(wxWindow *Parent, ALibraryContainer *LC);
  virtual ~THtml();

  void OnKeyDown(wxKeyEvent &event);  
  void OnChar(wxKeyEvent &event);  
  void OnNavigation(wxNavigationKeyEvent &event);  

  void SetSwitchState(THtmlSwitch &sw, size_t state);

  int GetBorders() const {  return wxHtmlWindow::m_Borders;  }
  void SetFonts(const olxstr &normal, const olxstr &fixed )  {
    this->NormalFont = normal;
    this->FixedFont = fixed;
    wxHtmlWindow::SetFonts( normal.u_str(), fixed.u_str() );
  }
  void GetFonts(olxstr &normal, olxstr &fixed)  {
    normal = this->NormalFont;
    fixed = this->FixedFont;
  }

  bool GetShowTooltips()  const {  return ShowTooltips;  }
  void SetShowTooltips(bool v, const olxstr &html_name=EmptyString);

  bool IsPageLoadRequested() const {  return PageLoadRequested;  }
  inline void IncLockPageLoad()    {  LockPageLoad++;  }
  inline void DecLockPageLoad()    {  LockPageLoad--;  }
  inline bool IsPageLocked() const {  return LockPageLoad != 0;  }

  bool ProcessPageLoadRequest();

  const olxstr& GetHomePage() const   {  return HomePage;  }
  void SetHomePage(const olxstr& hp)  {  HomePage = hp;  }

  bool LoadPage(const wxString &File);
  bool ReloadPage();
  bool UpdatePage();
  DefPropC(olxstr, WebFolder)

  void CheckForSwitches(THtmlSwitch &Sender, bool IsZip);
  void UpdateSwitchState(THtmlSwitch &Switch, olxstr &String);
  THtmlSwitch& GetRoot() const {  return *Root; }
  bool ItemState(const olxstr &ItemName, short State);
  // object operations
  bool AddObject(const olxstr& Name, AOlxCtrl *Obj, wxWindow* wxObj, bool Manage = false);
  AOlxCtrl *FindObject(const olxstr& Name)  {
    const size_t ind = Objects.IndexOf(Name);
    return (ind == InvalidIndex) ? NULL : Objects.GetValue(ind).A();
  }
  wxWindow *FindObjectWindow(const olxstr& Name)  {
    const size_t ind = Objects.IndexOf(Name);
    return (ind == InvalidIndex) ? NULL : Objects.GetValue(ind).B();
  }
  inline size_t ObjectCount() const {  return Objects.Count();  }
  inline AOlxCtrl* GetObject(size_t i)  {  return Objects.GetValue(i).A();  }
  inline wxWindow* GetWindow(size_t i)  {  return Objects.GetValue(i).B();  }
  inline const olxstr& GetObjectName(size_t i) const {  return Objects.GetKey(i);  }
  inline bool IsObjectManageble(size_t i) const {  return Objects.GetValue(i).GetC();  }
  //
  DefPropBIsSet(Movable)

  TActionQueue *OnURL;
  TActionQueue *OnLink;

  TActionQueue *OnDblClick;
  TActionQueue *OnKey;
  TActionQueue *OnCmd;

  TWindowInterface WI;
  // global data for the HTML parsing....
  static olxstr SwitchSource;
  static str_stack SwitchSources;
  // an extention...
  class WordCell : public wxHtmlWordCell  {
  public:
    WordCell(const wxString& word, const wxDC& dc) : wxHtmlWordCell(word, dc)  {  }
    //just this extra function for managed alignment ...
    void SetDescent(int v) {  m_Descent = v;  }
  };

  DECLARE_EVENT_TABLE()
};
#endif
