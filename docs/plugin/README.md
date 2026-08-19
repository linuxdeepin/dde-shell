# DDE Shell Plugin Documentation

Welcome to the DDE Shell plugin development documentation. This directory contains comprehensive guides for creating plugins for DDE Shell.

## Documentation Files

### API Reference
- **[API_en.md](./API_en.md)** (English) - Complete API reference for plugin development

### Tutorials
- **[tutorial_en.md](./tutorial_en.md)** (English) - Step-by-step guide to creating plugins

## Chinese Documentation / 中文文档

For Chinese documentation / 中文文档：

- **[API_zh_CN.md](./API_zh_CN.md)** - Complete API reference / 完整的 API 参考
- **[tutorial_zh_CN.md](./tutorial_zh_CN.md)** - Step-by-step tutorial / 分步教程
- **[README_zh_CN.md](./README_zh_CN.md)** - Overview (Chinese) / 概览（中文）

---
## Plugin Types

DDE Shell supports three main types of plugins:

1. **Applet Plugin** - Simple, single-function plugins (widgets, indicators)
2. **Containment Plugin** - Container plugins that manage multiple child applets
3. **Panel Plugin** - Specialized containers for panels (dock, notification center)

## Getting Started

### Quick Start

If you're new to plugin development, start with the tutorial:

```bash
# 1. Read the tutorial
cat tutorial_en.md

# 2. Explore example plugins
ls ../../example/

# 3. Build and test an example
cd ../../example/applet-example
cmake -Bbuild
cmake --build build
```

### Plugin Development Workflow

1. **Choose plugin type** based on your needs
2. **Create directory structure** with required files
3. **Implement plugin class** inheriting from base class
4. **Write metadata.json** with plugin information
5. **Create CMakeLists.txt** for building
6. **Build and test** your plugin

## Examples

The `example/` directory contains working plugin examples:

| Example | Type | Description |
|---------|------|-------------|
| `applet-example` | QML Applet | Simple QML-based applet |
| `applet-widget-example` | Widget Applet | Qt Widgets-based applet |
| `containment-example` | Containment | Container for child applets |
| `panel-example` | Panel | Dock-style panel |

## Resources
## Build System

Plugins use CMake with custom macros:

- `ds_install_package()` - Install plugin package
- `D_APPLET_CLASS()` - Register plugin class
- Links against `dde-shell-frame` for base classes

## Troubleshooting

### Plugin Not Loading
1. Check `metadata.json` syntax is valid
2. Verify plugin ID is unique
3. Ensure plugin library is built and installed
4. Review logs for error messages

### QML Not Loading
1. Verify `Url` in metadata.json points to existing QML file
2. Check QML syntax is correct
3. Ensure all required imports are present
4. Review QML console logs

## Best Practices

- **Use unique plugin IDs** with reverse domain notation
- **Follow naming conventions** (e.g., `org.deepin.ds.myplugin`)
- **Handle errors gracefully** in `load()` and `init()` methods
- **Clean up resources** properly in destructors
- **Use Qt's parent-child system** for automatic cleanup
- **Test thoroughly** before deployment

## Support

- Review [API reference](./API_en.md)
- File issues on GitHub

## License

DDE Shell plugins follow GPL-3.0-or-later license unless otherwise specified.

---

**Happy plugin development! 🎉**
