# Layer aggregator. Each layer is its own subdirectory and target so the
# dependency rule (core/ links no Qt/Xlib/D-Bus) is enforced by the target
# graph. Layers are added here as they come to exist.
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(storage)
add_subdirectory(runtime)
add_subdirectory(adapters)
add_subdirectory(app)
