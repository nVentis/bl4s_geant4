# GlassBar simulation

This is an example of how to use Geant4 via the DD4hep simulation toolkit (a modern standard in high-energy physics) to model Cerenkov light produced by electrons passing through a lead glass cuboid and hitting a SiPM at the end.

# Setup

We assume CVMFS to be available. If you do not have access to it, install it and make sure the sw.hsf.org repository is available.

We use the key4hep software stack provided by CVMFS. You can load it by running

    source setup.sh

Now you have access to the `ddsim` command which we use for detector simulation, and `k4run` for reconstruction.

To make some additional plugins available, `cd plugins && mkdir build && cd build && cmake .. && make install`

# Model and Geometry

The geometry is defined in `compact/GlassBar.xml`, the other XML files in the same directory as well as `compact/GlassBar/GlassBar_Cherenkov.xml`.

Both the SiPM and the lead glass are modelled as cuboids using the `DD4hep_BoxSegment` class. Note that the SiPM is modelled as a continuous, 9.5cm x 9.5cm slap.

Take a look at `materials.xml` for the definition of the refractive indices (e.g. `n=1.47` for the lead glass).

# Running simulation

The following command

    ddsim --steeringFile steering.py -N 10

runs the simulation for 10 events. Take a look at the `steering.py` file to set simulation parameters.

A few important ones are:

    --outputFile "filename.root"
    --gun.direction "X_direction,Y_direction,Z_direction"
    --gun.energy "4*GeV" 
    [--storePhotonSteps]

where `(X_direction,Y_direction,Z_direction)` should be a unit-vector along the desired electron direction.

The defaults in `steering.py` include a direction of the electron gun along `(0, 0, 1)` (i.e. along z-direction), a position at the origin and an energy of 4 GeV.

The flag `--storePhotonSteps` is optional. If supplied, it turns on a plugin (see `plugins/src/EveryStepTrackerAction`) that saves the position of each created photon at each point, instead of just saving the initial and final position before/after total internal reflection (TIR).

## Custom detector plugins

For comparison studies, `LeadGlassBar` can optionally be wrapped in a reflective coating so Cherenkov photons that would otherwise leak out the side walls are guided down the bar by total internal reflection to the SiPM instead (this is *not* the default runtime configuration -- see "Reflective vs bare LeadGlassBar side walls" below). DD4hep can't bind an optical surface to one specific face/border of a volume through declarative compact XML -- the generic `DD4hep_BoxSegment` plugin (or any other XML-only detector) has no way to express it, since that only works via the `dd4hep::BorderSurface`/`SkinSurface` C++ API, called from a real detector constructor. A skin surface on the whole box isn't an option either: it would apply uniformly to every face, including the one touching the SiPM, blocking all light.

`plugins/src/BoxSegmentReflective_geo.cpp` is a small custom detector constructor (registered as `DD4hep_BoxSegmentReflective`) that places the box exactly like `DD4hep_BoxSegment` does, then additionally binds an already-declared `<opticalsurface>` (see the `<surface name="..."/>` child) as a border surface between the box and world only -- leaving the face bordering the SiPM untouched so photons can still transmit into it. The surface itself (`LeadGlassBar_SideMirror`) stays declared in `compact/GlassBar.xml` regardless of whether the reflective detector type is in use.

Build it once with:

    cmake -S plugins -B plugins/build
    cmake --build plugins/build

`source setup.sh` puts `plugins/build` on `LD_LIBRARY_PATH` automatically, so ddsim finds the `DD4hep_BoxSegmentReflective` factory without any extra steps if/when you switch to it. If you add more detectors needing this kind of surface, add their sources under `plugins/src/` and rebuild.

## Reflective vs bare LeadGlassBar side walls

The default in `compact/GlassBar/GlassBar_Cherenkov.xml` is bare, uncoated glass: `LeadGlassBar` uses plain `type="DD4hep_BoxSegment"` with no `<surface>` child, relying entirely on Geant4's default Fresnel/TIR physics (computed from the `LeadGlass`/`Air` `RINDEX` values in `compact/materials.xml`, no explicit surface needed). This matches the real detector, which is polished but has no reflective coating as far as we know.

A same-seed, 5-event A/B comparison (`--random.seed 12345`) showed:

| variant | SiPMHits/event | time/event |
|---|---|---|
| bare (`DD4hep_BoxSegment`) | 15,181 (1 event sampled) | ~111s |
| reflective (`DD4hep_BoxSegmentReflective`, 99.5% wall reflectivity) | 13,574 - 19,723 | ~12-15s |

SiPM yield is statistically the same either way -- the reflective wrap doesn't change the physics outcome, since TIR already happens correctly from Fresnel physics alone. The only real difference is runtime: the explicit `REFLECTIVITY` property on a `dielectric_dielectric` surface appears to let Geant4 skip the full per-bounce Fresnel angle calculation for a much cheaper probabilistic reflect/absorb draw, making the reflective variant ~8-9x faster. Neither number should be taken as final -- both came from small, quick comparison runs, not a proper statistics study.

To switch to the reflective variant, in `compact/GlassBar/GlassBar_Cherenkov.xml` change the `LeadGlassBar` detector's `type` to `DD4hep_BoxSegmentReflective` and add a child `<surface name="LeadGlassBar_SideMirror"/>` (both are still present, just disconnected, in the current file's comments/history). Make sure the plugin is built first (see above).