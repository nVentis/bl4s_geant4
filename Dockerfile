# Based on https://github.com/ILDAnaSoft/ZHH/blob/main/Dockerfile
FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest

SHELL ["/bin/bash", "-c"]

WORKDIR /bl4s_geant4
COPY . .

# Build the custom DDG4 plugins (EveryStepTrackerAction, BoxSegmentReflective)
# at image-build time, so the image is ready to run ddsim as-is. This needs
# CVMFS mounted (setup.sh sources the key4hep stack from it), which needs
# the BuildKit "insecure" entitlement -- see
# .github/workflows/test_simulation.yml for how CI enables it.
RUN --security=insecure \
    echo "Mounting CVMFS" && /mount.sh && \
    echo "Checking whether key4hep exists..." && \
    { [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found"; } || exit 1 && \
    source setup.sh && \
    cmake -S plugins -B plugins/build && \
    cmake --build plugins/build -j"$(nproc)" && \
    ls plugins/build/libGlassBarPlugins.so && echo "Plugin built. Done"

# CVMFS is a FUSE mount, so it doesn't persist into the image between build
# steps -- containers started from this image need to mount it again at
# startup, which /mount.sh (from the base image) does before exec-ing CMD.
ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]
