// HEX SDK

#ifndef HEX_FIRSTTIME_IMPL_H
#define HEX_FIRSTTIME_IMPL_H

#ifdef __cplusplus

#include <string>
#include <hex/firsttime_module.h>

namespace hex_firsttime {

enum NavOperation {
    STAY  = 0,
    QUIT  = -1,
    PREV  = -2,
    NEXT  = -3,
    LAST  = -4
};

class SetupModule {
public:
    SetupModule(int index, const char* name);

    virtual ~SetupModule();

    /**
     * The module's main function. This should query the user for whatever config
     * details are necessary and update the policy working-set if appropriate.
     *
     * Return value indicates how the wizard should proceed. In general, this
     * should be retrieved by prompting the user to select an action using
     * SetupReadListIndex with showNavChoices set to true
     */
    virtual NavOperation main() = 0;

    /**
     * A function to display summary information about this module. This must be
     * non-interactive. It should use information from the policy working-set when
     * appropriate.
     *
     * The default is to show nothing.
     */
    virtual void summary();

    /**
     * This function will be called with all the values from settings.sys. If
     * modules are interested in those values, they can override this function.
     *
     * The default is to do nothing.
     */
    virtual void parseSys(const char* name, const char* value);

    // Getter for the module's index
    int index() const;

    // Getter for the title
    const char* title() const;

    // Helper function to display the module's title
    virtual void printTitle() const;

    /**
     * If this function returns true, this module won't be displayed to the user and
     * execution will continue with the next module.
     *
     * This can be used to optionally show/hide module based on
     * previous user selections or other configuration.
     *
     * The default implementation returns false.
     */
    virtual bool skipModule();

protected:

    int m_index;

    std::string m_name;

};

/**
 * A first-time setup wizard module based around a single main menu with
 * navigation operations.
 */
class MenuModule : public SetupModule
{
public:

    MenuModule(int index, const char* name);

    virtual ~MenuModule();

    /**
     * The main loop. Handles loop setup, and then the loop of displaying items,
     * prompting the user for a choice and then branching. The basic flow is:
     */
    virtual NavOperation main();

protected:

    /**
     * This function is called once each time the main() function is invoked,
     * prior to the menu loop starting. It should perform any necessary pre-loop
     * setup.
     *
     * A return value of false will abort the wizard.
     *
     * The default implementation just returns true
     */
    virtual bool loopSetup();

    /**
     * This function is called once each time around the main table loop,
     * allowing implementing subclasses to display a banner with status, etc.
     *
     * The default implementation does nothing
     */
    virtual void printLoopHeader();

    /**
     * This function is used to handle the user's choice from the main menu.
     * It must be implemented. The index corresponds to a valid index from
     * m_menuItems
     *
     * The return value should be false if something catastrophic has happened
     * and the wizard has to abort, dumping the user back at the login prompt
     */
    virtual bool doAction(int index) = 0;

    /**
     * Add an option to the list that the user can choose.
     *
     * \param menu:  The text to display in the menu
     * \param title: The text to display as a title when the option is chosen
     */
    virtual void addOption(const std::string &menu, const std::string &title);

    NavOperation m_actionDefaultOnError;

protected:

    // The list of options to display in the main menu.
    CliList m_menuItems;

    // The list of headings to display when an option has been chosen
    CliList m_menuHeadings;
};

}

#endif /* __cplusplus */

#endif /* endif HEX_FIRSTTIME_IMPL_H */

