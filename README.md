# GlassBar simulation

This is an example of how to use Geant4 via the DD4hep simulation toolkit (a modern standard in high-energy physics) to model Cerenkov light produced by electrons passing through a lead glass cuboid and hitting a SiPM at the end.

# Setup

We assume CVMFS to be available. If you do not have access to it, install it and make sure the sw.hsf.org repository is available.

We use the key4hep software stack provided by CVMFS. You can load it by running

    source setup.sh

Now you have access to the `ddsim` command which we use for detector simulation, and `k4run` for reconstruction.

# Running simulation

The following command creates a 

    ddsim --compactFile compact/GlassBar.xml \
        --enableGun --gun.particle e- --gun.energy "4*GeV" \
        -N 10 --outputFile glassbar_test.root

The default options of the particle gun is to use `mu`ons (hence we set particle `e` for electrons), a direction along `(0, 0, 1)`, i.e. along z-direction and a position at the origin.