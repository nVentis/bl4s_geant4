//==========================================================================
// Same as DD4hep_BoxSegment, but optionally binds an already-declared
// <opticalsurface> (via a <surface name="..."/> child) as a border surface
// between this volume and its mother (assumed to be world/Air). Used for
// TIR light-guide bars where the side walls need a reflective treatment
// that a bare skin surface on the whole box would wrongly also apply to
// faces bordering a downstream sensor.
//==========================================================================
#include "DD4hep/DetFactoryHelper.h"
#include "DD4hep/OpticalSurfaces.h"

using namespace std;
using namespace dd4hep;
using namespace dd4hep::detail;

static Ref_t create_element(Detector& description, xml_h e, Ref_t sens)  {
  xml_det_t   x_det = e;
  string      name  = x_det.nameStr();
  xml_comp_t  box    (x_det.child(_U(box)));
  xml_dim_t   pos    (x_det.child(_U(position), false));
  xml_dim_t   rot    (x_det.child(_U(rotation), false));
  Material    mat    (description.material(x_det.materialStr()));
  DetElement  det    (name,x_det.id());
  Volume      det_vol(name+"_vol",Box(box.x(),box.y(),box.z()), mat);
  Volume      mother = description.pickMotherVolume(det);
  Transform3D transform;

  if ( pos && rot )   {
    transform = Transform3D(Rotation3D(RotationZYX(rot.z(),rot.y(),rot.x())),
			    Position(pos.x(),pos.y(),pos.z()));
  }
  else if ( rot )   {
    transform = Transform3D(Rotation3D(RotationZYX(rot.z(),rot.y(),rot.x())),
			    Position());
  }
  else if ( pos )   {
    transform = Transform3D(Rotation3D(), Position(pos.x(),pos.y(),pos.z()));
  }
  PlacedVolume phv = mother.placeVolume(det_vol,transform);
  det_vol.setVisAttributes(description, x_det.visStr());
  det_vol.setLimitSet(description, x_det.limitsStr());
  det_vol.setRegion(description, x_det.regionStr());
  if ( x_det.isSensitive() )   {
    SensitiveDetector sd = sens;
    xml_dim_t sd_typ = x_det.child(_U(sensitive));
    det_vol.setSensitiveDetector(sens);
    sd.setType(sd_typ.typeStr());
  }
  if ( x_det.hasAttr(_U(id)) )  {
    phv.addPhysVolID("system",x_det.id());
  }
  det.setPlacement(phv);

  xml_dim_t x_surf = x_det.child(_U(surface), false);
  if ( x_surf )   {
    OpticalSurfaceManager surfMgr = description.surfaceManager();
    OpticalSurface         surf   = surfMgr.opticalSurface(x_surf.nameStr());
    PlacedVolume       mother_pv  = description.world().placement();
    BorderSurface(description, det, name+"_border_out", surf, phv, mother_pv);
    BorderSurface(description, det, name+"_border_in",  surf, mother_pv, phv);
  }

  return det;
}

// first argument is the type from the xml file
DECLARE_DETELEMENT(DD4hep_BoxSegmentReflective,create_element)
