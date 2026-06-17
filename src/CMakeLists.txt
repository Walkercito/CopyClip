# Layer aggregator. Each layer is its own subdirectory and target so the
# dependency rule (core/ links no Qt/Xlib/D-Bus) is enforced by the target
# graph. Layers are added here as they come to exist; for Task 0.1 only core/
# is present.
add_subdirectory(core)
