#ifndef _MAIN_APP_HPP
#define _MAIN_APP_HPP

#ifndef WX_PRECOMP

#include <wx/wxprec.h>

#endif

class MainApp : public wxApp {
public:
    bool OnInit() override;
};

DECLARE_APP(MainApp)

#endif //_MAIN_APP_HPP
