# 0.9.2 - 2012.03.29
- Fix disabling and hiding actions (Aurelien Gateau)
- Avoid spamming dbus at startup (Aurelien Gateau)
- Do not print warnings when not necessary (Aurelien Gateau)

# 0.9.1 - 2012.03.26
- Add support for "opened" and "closed" events (Aurelien Gateau)
- Add support for icon-data (LP BUG 633339) (Christoph Spielmann)

# 0.9.0 - 2011.08.30
- Add support for the "Status" dbusmenu property. Will be used by appmenu-qt for LP BUG 737419 (Aurelien Gateau)
- Collapse multiple separators, get rid of starting and trailing separators (LP BUG 793339) (Aurelien Gateau)

# 0.8.3 - 2011.06.21
- If DBusMenuExporter is deleted, delete all DBusMenu instances which were working with it (Aurelien Gateau)
- Only show icons in menu if the platform allows them (Michael Terry)

# 0.8.2 - 2011.04.12
- Shortcut handling: Translate "+" into "plus" and "-" into "minus" (LP BUG 712565) (Aurelien Gateau)

# 0.8.1 - 2011.03.24
- Added target to build documentation with Doxygen (Aurelien Gateau)

# 0.8.0 - 2011.02.24
- Implements version 2 of the dbusmenu protocol (Aurelien Gateau)
- Merged support for KMenu titles (Aurelien Gateau)

# 0.7.0 - 2011.13.01
- Switched DBus domain from org.ayatana to com.canonical (Aurelien Gateau)

# 0.6.6 - 2010.12.08
- Properly increase version numbers (Aurelien Gateau)

# 0.6.5 - 2010.12.07
- Avoid false warnings (Aurelien Gateau)
- Make sure we don't track actions twice (KDE BUG 254066) (Aurelien Gateau)
- CMake-parser-friendly of dbusmenu_version.h (Aurelien Gateau)

# 0.6.4 - 2010.09.23
- Trigger action asynchronously when the "clicked" event is received (LP BUG 643393) (Aurelien Gateau)
- Fixed copyright headers (Aurelien Gateau)

# 0.6.3 - 2010.09.16
- Moved to LP (Aurelien Gateau)
- Removed all code which did not belong to Canonical. Hopefully we get this
  code back in soon (Aurelien Gateau)

# 0.6.2 - 2010.09.09
- Fix some memory leaks (Aurelien Gateau)
- Do not keep dangling pointers to deleted actions (LP BUG 624964) (Aurelien Gateau)
- Updated documentation of iconNameForAction() (Aurelien Gateau)

# 0.6.1 - 2010.09.02
- Fix some memory leaks (Aurelien Gateau)

# 0.6.0 - 2010.08.19
- Added the DBusMenuImporter::actionActivationRequested(QAction*) signal (Aurelien Gateau)
- Fix hardcoded libdir in pkgconfig file (LP BUG 610633) (oneforall)

# 0.5.2 - 2010.08.05
- Fix implementation of GetGroupProperties() (Aurelien Gateau)
- Fix detection of QIcon::name() with gold (Aurelien Gateau)

# 0.5.1 - 2010.07.01
- Add support for KMenu titles (Christoph Feck)

# 0.5.0 - 2010.06.28
- Queue calls to refresh() because we may be spammed with many LayoutUpdated()
  signals at once (Aurelien Gateau)
- Turned DBusMenuImporter::updateMenu() into a slot (Aurelien Gateau)

# 0.4.0 - 2010.06.24
- Introduce a dbusmenu_version.h file (Aurelien Gateau)
- Introduce updateMenu() and menuUpdated(), deprecate menuReadyToBeShown() (Aurelien Gateau)
- Better build check for QIcon::name() (LP BUG 597975) (Aurelien Gateau)

# 0.3.5 - 2010.06.17
- Rework the way menuReadyToBeShown() is emitted (Aurelien Gateau)
- Queue LayoutUpdated() signal to avoid emitting it too often (Aurelien Gateau)
- Increase timeouts: prefer slow but complete menus to fast but incomplete (Aurelien Gateau)
- Use QIcon::name() to return the icon name, when built with Qt 4.7 (Aurelien Gateau)
- Correctly handle non-exclusive action groups (Aurelien Gateau)

# 0.3.4 - 2010.06.10
- Fixed KDE bug #237156 (Aurelien Gateau)
- Added support for shortcuts (Aurelien Gateau)
- Make the connection to LayoutUpdated() work :/ (Aurelien Gateau)

# 0.3.3 - 2010.05.19
- Introduce a qt minimum version. Qt 4.5 doesn't work. (Michael Jansen)
- Use the FindQjson.cmake file made by pinotree for chokoq because it works.
  The old one didn't (for me). (Michael Jansen)
- Refresh after LayoutUpdated signal (Marco Martin)
- Test items added to an existing menu are properly imported (Aurelien Gateau)
- Allow notification of the menu being filled, don't call aboutToShow more than
  once per actual menu showing (Aaron Seigo)
- Win32 fixes from Ralf Habacker (Patrick Spendrin)
- Added option to disable tests (Alex Elsayed)

# 0.3.2 - 2010.04.02
- Fix some weird positioning of menus and submenus.
- Handle ItemPropertyUpdated() signal.
- Correctly handle properties which are not part of the returned property map
  because they are set to their default value.
- Export "visible" property of action.

# 0.3.1 - 2010.04.01
- Updated to the latest protocol change: 0 is no longer the default value of
  the "toggle-state" property.
- Make it build without QJson again.

# 0.3.0 - 2010.03.09
- Moved the DBus side of DBusMenuExporter to a separate class, hiding it from
  the outside world.
- Cleaned the API of DBusMenuExporter and DBusMenuImporter.
- Implemented AboutToShow method from the DBusMenu spec.

# 0.2.2 - 2010.02.17
- Fixed crash if action is removed from menu after exporter is deleted
  (LP BUG 521011).
- Introduced a Qt equivalent of the test application used by dbusmenu-bench in
  libdbusmenu-glib.
- Added distcheck target.

# 0.2.1 - 2010.02.04
- Export KDE titles as disabled standard items.
- Added temporary workaround to get more dynamic menus to work on GNOME.

# 0.2.0 - 2010.02.03
- Make it possible to define the object-path used by DBusMenuExporter.
- Unit tests.
- Follow new DBusMenu spec.

# 0.1.0 - 2010.01.05
- First release.
