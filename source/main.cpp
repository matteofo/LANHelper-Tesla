#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

#include "ipglobal.hpp"
#include "xlink.hpp"

#include <iostream>
#include <lanhelper.h>

#define TITLE "LAN Play IP Generator"
#define VERSION "v1.1.0"

Result genRes = 0;

class MainGui;
tsl::Overlay* ovlHandle;

class GenFailureGui : public tsl::Gui {
public:
    GenFailureGui() {}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame(TITLE, VERSION);
        auto list = new tsl::elm::List();

        std::string label = "IP assign failed! Result code: " + std::format("{:x}", genRes);

        list->addItem(new tsl::elm::CategoryHeader(label), true);

        frame->setContent(list);

        return frame;
    }
};

class GenResultGui : public tsl::Gui {
public:
    GenResultGui() {}

    virtual tsl::elm::Element* createUI() override {
        int ip_int[4];
        char ip_str[15];

        genIp(ip_str, ip_int);

        auto frame = new tsl::elm::OverlayFrame(TITLE, VERSION);
        auto list = new tsl::elm::List();

        std::string label = "Generated IP address: " + std::string(ip_str);

        // Text layout from:
        // https://github.com/averne/Fizeau/blob/master/overlay/src/gui.cpp
        auto* drawer = new tsl::elm::CustomDrawer([label](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(label.c_str(), false, x, y + 20, 20, renderer->a(0xffff));
        });

        list->addItem(drawer, 30);

        auto* doneButton = new tsl::elm::ListItem("Apply!");
        doneButton->setClickListener([ip_int, list, frame](u64 keys) {
            if (keys & HidNpadButton_A) {
                Result res = setIp(ip_int);

                if(R_SUCCEEDED(res)) {
                    tsl::changeTo<MainGui>();
                } else {
                    genRes = res;
                    tsl::changeTo<GenFailureGui>();
                }

                return true;
            }

            return false;
        });

        list->addItem(doneButton);

        frame->setContent(list);

        return frame;
    }
};

class XLinkGenResultGui : public tsl::Gui {
public:
    XLinkGenResultGui() {}

    virtual tsl::elm::Element* createUI() override {
        ipglobal::mac_addr hwaddr = ipglobal::getMacAddress();

        if(hwaddr.p1 == 0) {
            tsl::changeTo<GenFailureGui>();
        }

        std::string ipstr = xlink::getIPString(hwaddr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, VERSION);
        auto list = new tsl::elm::List();

        std::string ipLabel = "Applying IP address " + ipstr;
        std::string macLabel = "(MAC Digits: " + ipglobal::getPartialMacString(hwaddr) + ")";

        // Text layout from:
        // https://github.com/averne/Fizeau/blob/master/overlay/src/gui.cpp
        auto* drawer = new tsl::elm::CustomDrawer([ipLabel, macLabel](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(ipLabel.c_str(), false, x, y + 20, 20, renderer->a(0xffff));
            renderer->drawString(macLabel.c_str(), false, x, y + 45, 20, renderer->a(0xffff));
        });

        list->addItem(drawer, 60);

        auto* doneButton = new tsl::elm::ListItem("Apply!");
        doneButton->setClickListener([hwaddr, list, frame](u64 keys) {
            if (keys & HidNpadButton_A) {
                Result res = xlink::setIPAddress(hwaddr);

                if(R_SUCCEEDED(res)) {
                    tsl::changeTo<MainGui>();
                } else {
                    genRes = res;
                    tsl::changeTo<GenFailureGui>();
                }

                return true;
            }

            return false;
        });

        list->addItem(doneButton);
        frame->setContent(list);

        return frame;
    }
};

class MainGui : public tsl::Gui {
public:
    MainGui() {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame(TITLE, VERSION);

        // A list that can contain sub ellements and handles scrolling
        auto list = new tsl::elm::List();

        // this crashes the console so nuh uh
        // list->setClickListener([](u64 keys) {
        //     if(keys & HidNpadButton_B) {
        //         exit(0);
        // 
        //         return true;
        //     }
        // 
        //     return false;
        // });

        auto* generateButton = new tsl::elm::ListItem("Set Lan Play IP");
        generateButton->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<GenResultGui>();

                return true;
            } else if (keys & HidNpadButton_B) {
                ovlHandle->close();

                return true;
            }

            return false;
        });

        auto* generateXLinkButton = new tsl::elm::ListItem("Set XLink Kai IP");
        generateXLinkButton->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<XLinkGenResultGui>();

                return true;
            } else if (keys & HidNpadButton_B) {
                ovlHandle->close();

                return true;
            }

            return false;
        });

        auto* resetButton = new tsl::elm::ListItem("Reset IP Config (DHCP + 90DNS)");
        resetButton->setClickListener([this](u64 keys) {
            if (keys & HidNpadButton_A) {
                defaultIP();

                return true;
            } else if (keys & HidNpadButton_B) {
                ovlHandle->close();

                return true;
            }

            return false;
        });

        // Create and add a new list item to the list
        list->addItem(generateButton);
        list->addItem(generateXLinkButton);
        list->addItem(resetButton);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        return false;   // Return true here to signal the inputs have been consumed
    }
};

class LANHelperOverlay : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {
        nifmInitialize(NifmServiceType_Admin);
        setInitialize();
        setcalInitialize();
    }  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {
        nifmExit();
        setcalExit();
        setExit();
    }  // Called at the end to clean up all services previously initialized

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        ovlHandle = this;
        return initially<MainGui>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

int main(int argc, char **argv) {
    return tsl::loop<LANHelperOverlay>(argc, argv);
}
