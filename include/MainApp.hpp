#ifndef _MAIN_APP_HPP
#define _MAIN_APP_HPP

#include "wx/wxprec.h"

class MainApp : public wxApp {
public:
    bool OnInit() override;
};

DECLARE_APP(MainApp)

#endif //_MAIN_APP_HPP
