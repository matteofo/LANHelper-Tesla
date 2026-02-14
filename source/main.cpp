#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

#include "ipglobal.hpp"
#include "xlink.hpp"

#include <iostream>
#include <lanhelper.h>

#define TITLE "LAN Helper"
#define CLEANUP_LABEL   "Warning! This operation will remove\nall wired and \"ghost\" (unnamed)\nnetwork connections.\n\n" \
                        "If there are any network settings\nyou'd like to save, please\ndo so now!\n\n" \
                        "To apply the changes,\nyou'll have to restart your Switch."
#define CLEANUP_DONE_LABEL  "Done. Would you like to reboot\nyour Switch now? (changes won't\nbe applied until you do.)"

Result genRes = 0;

class MainGui;
tsl::Overlay* ovlHandle;

class CleanupDoneGui : public tsl::Gui {
public:
    CleanupDoneGui() {}

    virtual tsl::elm::Element* createUI() override {
        ipglobal::ip_addr addr = ipglobal::getIPAddress();
        std::string addrString = ipglobal::getIPString(addr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, std::format("{} ({})", APP_VERSION, addrString).c_str());
        auto list = new tsl::elm::List();

        Result res = ipglobal::cleanup();
        if (R_FAILED(res)) {
            std::string message = std::format("Failed to clean up\nnetwork interfaces! (error code 0x{:x})", R_VALUE(res));
        
            auto* drawer = new tsl::elm::CustomDrawer([message](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawString(message.c_str(), false, x, y + 20, 20, renderer->a(0xffff));
            });

            list->addItem(drawer, 30);
            frame->setContent(list);

            return frame;
        }

        auto* drawer = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(CLEANUP_DONE_LABEL, false, x, y + 20, 20, renderer->a(0xffff));
        });

        auto yesButton = new tsl::elm::ListItem("Reboot now");
        yesButton->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) {
                appletRequestToReboot();
                return true;
            }

            return false;
        });

        auto noButton = new tsl::elm::ListItem("Reboot later");
        noButton->setClickListener([](u64 keys) {
            if ((keys & HidNpadButton_A) || (keys & HidNpadButton_B)) {
                ovlHandle->close();
                return true;
            }

            return false;
        });

        list->addItem(drawer, 80);
        list->addItem(yesButton);
        list->addItem(noButton);

        frame->setContent(list);
        return frame;
    }
};

class CleanupGui : public tsl::Gui {
public:
    CleanupGui() {}

    virtual tsl::elm::Element* createUI() override {
        ipglobal::ip_addr addr = ipglobal::getIPAddress();
        std::string addrString = ipglobal::getIPString(addr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, std::format("{} ({})", APP_VERSION, addrString).c_str());
        auto list = new tsl::elm::List();

        // Text layout from:
        // https://github.com/averne/Fizeau/blob/master/overlay/src/gui.cpp
        auto* drawer = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(CLEANUP_LABEL, false, x, y + 20, 20, renderer->a(0xffff));
        });

        auto button = new tsl::elm::ListItem("Continue");
        button->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<CleanupDoneGui>();
                return true;
            }

            return false;
        });


        list->addItem(drawer, 220);
        list->addItem(button);

        frame->setContent(list);
        return frame;
    }
};

class GenFailureGui : public tsl::Gui {
public:
    GenFailureGui() {}

    virtual tsl::elm::Element* createUI() override {
        ipglobal::ip_addr addr = ipglobal::getIPAddress();
        std::string addrString = ipglobal::getIPString(addr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, std::format("{} ({})", APP_VERSION, addrString).c_str());
        auto list = new tsl::elm::List();

        std::string label = "IP assign failed! Result code: " + std::format("0x{:x}, {}, {}", genRes, R_DESCRIPTION(genRes), R_MODULE(genRes));

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

        ipglobal::ip_addr addr = ipglobal::getIPAddress();
        std::string addrString = ipglobal::getIPString(addr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, std::format("{} ({})", APP_VERSION, addrString).c_str());
        auto list = new tsl::elm::List();

        std::string titleLabel = "Your LAN Play IP address:";

        // Text layout from:
        // https://github.com/averne/Fizeau/blob/master/overlay/src/gui.cpp
        auto* drawer = new tsl::elm::CustomDrawer([titleLabel, ip_str](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(titleLabel.c_str(), false, x, y + 20, 26, renderer->a(0xffff));
            renderer->drawString(ip_str, false, x, y + 48, 20, renderer->a(0xffff));
        });

        list->addItem(drawer, 60);

        auto* doneButton = new tsl::elm::ListItem("Apply!");
        doneButton->setClickListener([ip_int, list, frame](u64 keys) {
            if (keys & HidNpadButton_A) {
                Result res = setIp(ip_int);

                if(R_SUCCEEDED(res)) {
                    ovlHandle->close();
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

        ipglobal::ip_addr addr = ipglobal::getIPAddress();
        std::string addrString = ipglobal::getIPString(addr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, std::format("{} ({})", APP_VERSION, addrString).c_str());
        auto list = new tsl::elm::List();

        std::string titleLabel = "Your XLink Kai IP address:";

        // Text layout from:
        // https://github.com/averne/Fizeau/blob/master/overlay/src/gui.cpp
        auto* drawer = new tsl::elm::CustomDrawer([titleLabel, ipstr](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(titleLabel.c_str(), false, x, y + 20, 26, renderer->a(0xffff));
            renderer->drawString(ipstr.c_str(), false, x, y + 48, 20, renderer->a(0xffff));
        });

        list->addItem(drawer, 60);

        auto* doneButton = new tsl::elm::ListItem("Apply!");
        doneButton->setClickListener([hwaddr, list, frame](u64 keys) {
            if (keys & HidNpadButton_A) {
                Result res = xlink::setIPAddress(hwaddr);

                if(R_SUCCEEDED(res)) {
                    ovlHandle->close();
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
        ipglobal::ip_addr addr = ipglobal::getIPAddress();
        std::string addrString = ipglobal::getIPString(addr);

        auto frame = new tsl::elm::OverlayFrame(TITLE, std::format("{} ({})", APP_VERSION, addrString).c_str());

        // A list that can contain sub ellements and handles scrolling
        auto list = new tsl::elm::List();

        auto* generateButton = new tsl::elm::ListItem("Use Lan Play");
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

        auto* generateXLinkButton = new tsl::elm::ListItem("Use XLink Kai");
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
                ovlHandle->close();
                return true;
            } else if (keys & HidNpadButton_B) {
                ovlHandle->close();
                return true;
            }

            return false;
        });

        auto* cleanupButton = new tsl::elm::ListItem("Clean up networks");
        cleanupButton->setClickListener([this](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<CleanupGui>();
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
        list->addItem(cleanupButton);

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
        setsysInitialize();
        setcalInitialize();
    }  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {
        nifmExit();
        setsysExit();
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
