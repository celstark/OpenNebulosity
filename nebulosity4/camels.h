#ifdef CAMELS
extern int CamelCorrect(fImage& Image);
extern void CamelDrawBadRect(fImage& Image, int x1, int y1, int x2, int y2);
extern void CamelAnnotate(wxBitmap& bmp);
extern void CamelGetScriptInfo(wxString raw_fname);
extern bool CamelCorrectActive;
extern int CamelCompression;
extern wxString CamelText;
extern wxString	CamelFName;
extern wxString CamelScriptInfo;
#endif