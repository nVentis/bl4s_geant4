//==========================================================================
// Sensitive action for LeadGlassBar used by --storePhotonSteps (see
// steering.py). Every built-in DDG4 tracker action (Geant4TrackerAction,
// Geant4TrackerWeightedAction with CollectSingleDeposits, ...) only ever
// creates a hit for a step that has a nonzero energy deposit. That's fine
// for the electron (continuous ionization deposits at every step), but
// useless for optical photons: TIR reflection and refraction at the bar's
// side walls are elastic, zero-deposit steps, so those actions silently
// drop every bounce and only ever see a photon's single final absorption
// step (if any). This action instead creates a hit for EVERY step of
// EVERY opticalphoton track unconditionally, so the full bounce-by-bounce
// path can be reconstructed as a true polyline; charged particles (the
// electron) keep the usual deposit-gated behaviour so their dE/dx
// trajectory is unaffected. Hits land in the same LeadGlassBarHits
// collection/schema as the default action, so existing downstream code
// doesn't need to change.
//
// LeadGlass's ABSLENGTH (materials.xml) is a soft, statistical cutoff
// (survival ~ exp(-pathLength/ABSLENGTH)); with untreated side walls a
// trapped photon can realistically bounce hundreds of times before it
// happens to be absorbed, and recording every one of those steps for
// every one of ~10^4 photons/event is what actually blows up memory (this
// was measured: unbounded, RSS hit ~4.5GB in under a minute for a single
// event). MaxHitsPerTrack and MaxOpticalTracks bound that independently
// of the underlying physics -- they only decide how much of the tail we
// bother recording/how many tracks we bother with, not whether a photon
// keeps bouncing or gets absorbed.
//==========================================================================
#include "DDG4/Geant4SensDetAction.h"
#include "DDG4/Geant4Data.h"
#include "DDG4/Geant4StepHandler.h"
#include "DDG4/Factories.h"

#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4Track.hh"

#include <unordered_map>

namespace dd4hep {
  namespace sim {

    class EveryStepTrackerAction : public Geant4Sensitive {
    protected:
      std::size_t m_collectionID { 0 };
      /// Property: stop recording further steps for a track once it has
      /// this many hits (still lets the track itself keep propagating).
      int m_maxHitsPerTrack { 500 };
      /// Property: only ever record the first N distinct opticalphoton
      /// tracks seen in an event; every later one is skipped entirely.
      int m_maxOpticalTracks { 200 };
      /// Per-event bookkeeping: trackID -> hits recorded so far.
      std::unordered_map<int, int> m_hitsPerTrack;

    public:
      EveryStepTrackerAction(Geant4Context* ctxt, const std::string& nam,
                              DetElement det, Detector& description)
        : Geant4Sensitive(ctxt, nam, det, description)
      {
        declareProperty("MaxHitsPerTrack", m_maxHitsPerTrack);
        declareProperty("MaxOpticalTracks", m_maxOpticalTracks);
        m_collectionID = defineCollection<Geant4Tracker::Hit>(m_sensitive.readout().name());
      }

      virtual ~EveryStepTrackerAction() {}

      virtual void begin(G4HCofThisEvent* hce) override {
        Geant4Sensitive::begin(hce);
        m_hitsPerTrack.clear();
      }

      virtual bool process(const G4Step* step, G4TouchableHistory* /* history */) override {
        Geant4StepHandler h(step);
        G4Track* trk = step->GetTrack();
        bool is_optical = (trk->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition());
        double edep = h.deposit();
        if ( !is_optical && edep <= 0.0 )
          return false;

        int track_id = trk->GetTrackID();
        if ( is_optical ) {
          auto it = m_hitsPerTrack.find(track_id);
          if ( it == m_hitsPerTrack.end() ) {
            if ( int(m_hitsPerTrack.size()) >= m_maxOpticalTracks )
              return false;
            it = m_hitsPerTrack.emplace(track_id, 0).first;
          }
          if ( it->second >= m_maxHitsPerTrack )
            return false;
          ++(it->second);
        }

        Geant4Tracker::Hit* hit = new Geant4Tracker::Hit(
            track_id,
            trk->GetDefinition()->GetPDGEncoding(),
            edep,
            step->GetPostStepPoint()->GetGlobalTime(),
            h.stepLength(),
            h.postPos(),
            h.postMom());
        hit->cellID = cellID(step);
        collection(m_collectionID)->add(hit);
        mark(step);
        return true;
      }
    };

  }
}

DECLARE_GEANT4SENSITIVE(EveryStepTrackerAction)
