/***************************************************************************************\
* DD4hep geometry code for the central drift chamber of the IDEA detector               *
* Author: Lorenzo Capriotti, Modified by Brieuc Francois to have sensitive cell volumes *
\***************************************************************************************/
#include "DD4hep/DetFactoryHelper.h"
#include "DD4hep/Printout.h"
#include "DD4hep/detail/DetectorInterna.h"
#include "TClass.h"
#include "TMath.h"
#include "XML/Utilities.h"
#include <XML/Layering.h>
#include <iostream>
#include <math.h>

using namespace std;
using namespace dd4hep;

struct wire
{
  dd4hep::Volume mother_volume;
  string type;
  int num;
  double radius;
  double phi;
  double phioffset;
  double stereo;
  double halfalpha;
  double thickness;
  double halflength;
  dd4hep::Volume volume;
  string name;
};

namespace {

struct CDCHBuild : public dd4hep::xml::tools::VolumeBuilder {
  std::vector<dd4hep::DetElement> deSuperLayer, deLayer, deSWire;

  CDCHBuild(dd4hep::Detector& description, xml_elt_t e, dd4hep::SensitiveDetector sens);

  double diff_of_squares(double a, double b);
  void apply_wire_coating(struct wire& w, double outwrap, double halflength, string material);
  void PlaceWires(struct wire& w, double outwrap, double halflength, int copyNunOffset, int SL, int iring);
  void build_layer(DetElement parent, Volume parentVol, dd4hep::SensitiveDetector sens);
};

// ******************************************************
// Initializing constructor
// ******************************************************

CDCHBuild::CDCHBuild(dd4hep::Detector& dsc, xml_elt_t e, dd4hep::SensitiveDetector sens)
    : dd4hep::xml::tools::VolumeBuilder(dsc, e, sens) {}

double CDCHBuild::diff_of_squares(double a, double b) {

  double diff = pow(a, 2) - pow(b, 2);
  return diff;
}

void CDCHBuild::apply_wire_coating(struct wire& w, double outwrap, double halflength, string material = "G4_Au"){
  dd4hep::Tube WrapTube(w.thickness, w.thickness + 0.5 * outwrap, halflength);
  dd4hep::Volume lvWireWrapVol(w.name + "_coating", WrapTube, description.material(material));
  dd4hep::Tube TotalWire(0.0, w.thickness + 0.5 * outwrap, halflength);
  dd4hep::Volume lvWireVol(w.name + "_totalWire", TotalWire, description.material("Air"));
  lvWireVol.placeVolume(w.volume, dd4hep::Position(0.0, 0.0, 0.0));
  lvWireVol.placeVolume(lvWireWrapVol, dd4hep::Position(0.0, 0.0, 0.0));
  w.volume = lvWireVol;
}

// deprecated function, only use it for the wires before and after the sensitive zone (mainly guard wires)
void CDCHBuild::PlaceWires(struct wire& w, double outwrap, double halflength, int copyNunOffset = 0, int SL = 999,
                           int iring = 999) {

  dd4hep::RotationZYX rot(0., 0., w.stereo);
  dd4hep::RotationX rot_stereo(w.stereo);
  dd4hep::Translation3D transl(w.radius, 0., 0.);

  dd4hep::Transform3D T(transl * rot_stereo);

  string wirewrapname = "lvWireWrap_SL";
  wirewrapname += std::to_string(SL);
  wirewrapname += "_ring";
  wirewrapname += std::to_string(iring);
  wirewrapname += "_type";
  wirewrapname += w.type;
  wirewrapname += "_stereo";
  wirewrapname += std::to_string(w.stereo);

  //cout << "wirewrapname: " << wirewrapname << endl;

  string wirename = "lvWire_SL";
  wirename += std::to_string(SL);
  wirename += "_ring";
  wirename += std::to_string(iring);
  wirename += "_type";
  wirename += w.type;
  wirename += "_stereo";
  wirename += std::to_string(w.stereo);

  apply_wire_coating(w, outwrap, halflength);

  //    registerVolume(lvWireWrapVol.name(), lvWireWrapVol);
  //    registerVolume(lvWireVol.name(), lvWireVol);

  // repeat the placement of wires over phi
  for (int n = 0; n < w.num; n++) {
    dd4hep::RotationZ iRot(w.phioffset + w.phi * n);
    if (n % 1 == 0) w.mother_volume.placeVolume(w.volume, dd4hep::Transform3D(iRot * T));
  }
}

void CDCHBuild::build_layer(DetElement parent, Volume parentVol, dd4hep::SensitiveDetector sens_det) {

  // ******************************************************
  // Loading parameters
  // ******************************************************

  double halfalpha = 0.5 * dd4hep::_toDouble("CDCH:alpha");
  double inner_radius = dd4hep::_toDouble("CDCH:r0");
  double outer_radius = dd4hep::_toDouble("CDCH:rOut");
  double halflength = dd4hep::_toDouble("CDCH:zHalfLength");
  double CarbonInnerWallThick = dd4hep::_toDouble("CDCH:CarbonInnerWallThick");
  double CopperInnerWallThick = dd4hep::_toDouble("CDCH:CopperInnerWallThick");
  double GasInnerWallThick = dd4hep::_toDouble("CDCH:GasInnerWallThick");
  double Carbon1OuterWallThick = dd4hep::_toDouble("CDCH:Carbon1OuterWallThick");
  double Carbon2OuterWallThick = dd4hep::_toDouble("CDCH:Carbon2OuterWallThick");
  double CopperOuterWallThick = dd4hep::_toDouble("CDCH:CopperOuterWallThick");
  double FoamOuterWallThick = dd4hep::_toDouble("CDCH:FoamOuterWallThick");
  double GasEndcapWallThick = dd4hep::_toDouble("CDCH:GasEndcapWallThick");
  double CopperEndcapWallThick = dd4hep::_toDouble("CDCH:CopperEndcapWallThick");
  double KaptonEndcapWallThick = dd4hep::_toDouble("CDCH:KaptonEndcapWallThick");
  double CarbonEndcapWallThick = dd4hep::_toDouble("CDCH:CarbonEndcapWallThick");
  double FWireShellThickIn = dd4hep::_toDouble("CDCH:FWireShellThickIn");
  double FWireShellThickOut = dd4hep::_toDouble("CDCH:FWireShellThickOut");
  double SWireShellThickIn = dd4hep::_toDouble("CDCH:SWireShellThickIn");
  double SWireShellThickOut = dd4hep::_toDouble("CDCH:SWireShellThickOut");
  double CntFWireShellThickIn = dd4hep::_toDouble("CDCH:CntFWireShellThickIn");
  double CntFWireShellThickOut = dd4hep::_toDouble("CDCH:CntFWireShellThickOut");
  double InGWireShellThickIn = dd4hep::_toDouble("CDCH:InGWireShellThickIn");
  double InGWireShellThickOut = dd4hep::_toDouble("CDCH:InGWireShellThickOut");
  double OutGWireShellThickIn = dd4hep::_toDouble("CDCH:InGWireShellThickIn");
  double OutGWireShellThickOut = dd4hep::_toDouble("CDCH:InGWireShellThickOut");
  double secure = dd4hep::_toDouble("CDCH:secure");
  double capGasLayer = dd4hep::_toDouble("CDCH:capGasLayer");
  double extShiftFW = dd4hep::_toDouble("CDCH:extShiftFW");
  double cellDimension = dd4hep::_toDouble("CDCH:cellDimension");
  double inGuardRad = dd4hep::_toDouble("CDCH:inGuardRad");
  double outGuardRad = dd4hep::_toDouble("CDCH:outGuardRad");
  int nSDeltaWire = dd4hep::_toInt("CDCH:nSDeltaWire");
  int nSWire = dd4hep::_toInt("CDCH:nSWire");
  int nInGWire = dd4hep::_toInt("CDCH:nInGWire");
  int nOutGWire = dd4hep::_toInt("CDCH:nOutGWire");
  int nStoFWireRatio = dd4hep::_toInt("CDCH:nStoFWireRatio");
  int nVerticalFWire = dd4hep::_toInt("CDCH:nVerticalFWire");
  int nSuperLayer = dd4hep::_toInt("CDCH:nSuperLayer");
  int nRing = dd4hep::_toInt("CDCH:nRing");
  int nFieldWireShells = dd4hep::_toInt("CDCH:nFieldWireShells");
  //bool setWireSensitive = true; // FIXME: add the possibility to have wires sensitive (parameter in the xml) which could be useful for detailed chamber behavior studies, current attempt never lead to a hit in the wire, even with enlarged wires...

  double epsilon = 0.0;
  double phi_ring = 0.0;
  double phi_ring1 = 0.0;
  int nFWire = 0;
  int nFWire1 = 0;
  int num_wire = 0;
  int nHorizontalFWire = nStoFWireRatio - nVerticalFWire;
  int sign_epsilon = -1;
  double phi = 0.0;
  double scaleFactor = 0.0;
  double dropFactor = 0.0;
  double epsilonFactor = 0.0;
  double delta_radius_ring = cellDimension;
  double senseWireRing_radius_0 = 0.0;
  double iradius = 0.0;
  double idelta_radius = 0.0;

  double envelop_Inner_thickness = CarbonInnerWallThick + CopperInnerWallThick + GasInnerWallThick;
  double envelop_Outer_thickness =
      Carbon1OuterWallThick + Carbon2OuterWallThick + CopperOuterWallThick + FoamOuterWallThick;
  double FWireDiameter = FWireShellThickIn + FWireShellThickOut;
  double FWradii = 0.5 * FWireDiameter;
  double CntFWireDiameter = CntFWireShellThickIn + CntFWireShellThickOut;
  double CntFWradii = 0.5 * CntFWireDiameter;
  double SWireDiameter = SWireShellThickIn + SWireShellThickOut;
  double SWradii = 0.5 * SWireDiameter;
  double inGWireDiameter = InGWireShellThickIn + InGWireShellThickOut;
  double inGWradii = 0.5 * inGWireDiameter;
  double fakeLayerInIWthick = -0.0001 + GasInnerWallThick;
  double inner_radius_0 = inner_radius + envelop_Inner_thickness - fakeLayerInIWthick;

  double radius_ring_0 = inner_radius + envelop_Inner_thickness + FWradii + secure + capGasLayer;
  double radius_ringOut_0 = radius_ring_0 - FWradii - secure;

  double drop = 0.0;
  double radius_ring = 0.0;
  double radius_ringIn_0 = 0.0;
  double radius_ringIn = 0.0;
  double radius_ringOut = 0.0;
  double epsilonIn = 0.0;
  double epsilonOut = 0.0;
  double ringangle = 0.0;
  double cellBase = 0.0;
  double inscribedRadius = 0.0;
  double circumscribedRadius = 0.0;
  double zlength = 0.0;
  double cellStaggering = 0.0;
  double epsilonInGwRing = 0.0;
  double epsilonOutGwRing = 0.0;
  double radius_ringIn_whole_cell = 0.0;
  double epsilonIn_whole_cell = 0.0;

  //------------------------------------------------------------------------
  // The enlarge parameter is used to see the wires in the rendering
  //------------------------------------------------------------------------

  double enlarge = 1.;

  //------------------------------------------------------------------------
  // Build the inner, outer and endcap walls first
  //------------------------------------------------------------------------

  dd4hep::Tube Endcap_Gas(inner_radius, outer_radius, 0.5 * GasEndcapWallThick);
  dd4hep::Tube Endcap_Copper(inner_radius, outer_radius, 0.5 * CopperEndcapWallThick);
  dd4hep::Tube Endcap_Kapton(inner_radius, outer_radius, 0.5 * KaptonEndcapWallThick);
  dd4hep::Tube Endcap_Carbon(inner_radius, outer_radius, 0.5 * CarbonEndcapWallThick);

  dd4hep::Volume lvEndcapWallGas =
      dd4hep::Volume("lvEndcapWallGasVol", Endcap_Gas, description.material("GasHe_90Isob_10"));
  dd4hep::Volume lvEndcapWallCopper =
      dd4hep::Volume("lvEndcapWallCopperVol", Endcap_Copper, description.material("G4_Cu"));
  dd4hep::Volume lvEndcapWallKapton =
      dd4hep::Volume("lvEndcapWallKaptonVol", Endcap_Kapton, description.material("Kapton"));
  dd4hep::Volume lvEndcapWallCarbon =
      dd4hep::Volume("lvEndcapWallCarbonVol", Endcap_Carbon, description.material("CarbonFiber"));

  dd4hep::Tube InnerWall_Carbon(inner_radius, inner_radius + CarbonInnerWallThick, halflength);
  dd4hep::Tube InnerWall_Copper(inner_radius + CarbonInnerWallThick,
                                inner_radius + CarbonInnerWallThick + CopperInnerWallThick, halflength);
  dd4hep::Tube InnerWall_Gas(inner_radius + CarbonInnerWallThick + CopperInnerWallThick,
                             inner_radius + envelop_Inner_thickness, halflength);

  dd4hep::Volume lvInnerWallCarbon =
      dd4hep::Volume("lvInnerWallCarbonVol", InnerWall_Carbon, description.material("CarbonFiber"));
  dd4hep::Volume lvInnerWallCopper =
      dd4hep::Volume("lvInnerWallCopperVol", InnerWall_Copper, description.material("G4_Cu"));
  dd4hep::Volume lvInnerWallGas =
      dd4hep::Volume("lvInnerWallGasVol", InnerWall_Gas, description.material("GasHe_90Isob_10"));

  dd4hep::Tube OuterWall_Copper(outer_radius - envelop_Outer_thickness,
                                outer_radius - Carbon1OuterWallThick - Carbon2OuterWallThick - FoamOuterWallThick,
                                halflength);
  dd4hep::Tube OuterWall_Carbon1(outer_radius - Carbon1OuterWallThick - Carbon2OuterWallThick - FoamOuterWallThick,
                                 outer_radius - Carbon2OuterWallThick - FoamOuterWallThick, halflength);
  dd4hep::Tube OuterWall_Foam(outer_radius - Carbon2OuterWallThick - FoamOuterWallThick,
                              outer_radius - Carbon2OuterWallThick, halflength);
  dd4hep::Tube OuterWall_Carbon2(outer_radius - Carbon2OuterWallThick, outer_radius, halflength);

  dd4hep::Volume lvOuterWallCarbon1 =
      dd4hep::Volume("lvOuterWallCarbon1Vol", OuterWall_Carbon1, description.material("CarbonFiber"));
  dd4hep::Volume lvOuterWallCarbon2 =
      dd4hep::Volume("lvOuterWallCarbon2Vol", OuterWall_Carbon2, description.material("CarbonFiber"));
  dd4hep::Volume lvOuterWallCopper =
      dd4hep::Volume("lvOuterWallCopperVol", OuterWall_Copper, description.material("G4_Cu"));
  dd4hep::Volume lvOuterWallFoam =
      dd4hep::Volume("lvOuterWallFoamVol", OuterWall_Foam, description.material("GasHe_90Isob_10"));

  dd4hep::DetElement CDCHDetector(parent, "CDCH", parent.id());

  //------------------------------------------------------------------------
  // Now we are ready to loop over the SuperLayers and fill the gas volume!
  //------------------------------------------------------------------------

  std::vector<dd4hep::Volume> lvLayerVol;
  std::vector<dd4hep::Volume> lvFwireVol, lvGwireVol;

  string wirecol, gascol, wholeHyperboloidVolumeName;
  string lvFwireName, lvSwireName;

  struct wire ground_wires, field_wires_bottom, field_wires_center, field_wires_top, sense_wires;
  // nSuperLayer = 1;

  for (int SL = 0; SL < nSuperLayer; ++SL) {

    num_wire = nSWire + SL * nSDeltaWire;
    phi = 2. * TMath::Pi() / num_wire;
    nFWire = nHorizontalFWire * num_wire;
    phi_ring = 2. * TMath::Pi() / nFWire;
    nFWire1 = nFWire / 2;
    if (ceilf(nFWire1) != nFWire1)
      throw std::runtime_error("Error: Failed to build CDCH. Please make sure that '(nStoFWireRatio - nVerticalFWire) * (nSWire + SuperLayerIndex * nSDeltaWire)' is always an even number");
    phi_ring1 = 2.0 * phi_ring;
    scaleFactor = (1.0 + TMath::Pi() / num_wire) / (1.0 - TMath::Pi() / num_wire);
    dropFactor = (1.0 / cos(halfalpha) - 1.0);
    epsilonFactor = sin(halfalpha) / halflength;
    ringangle = -0.5 * phi;

    gascol = "vCDCH:Gas1";
    if (SL % 3 == 0)
      gascol = "vCDCH:Gas1";
    else if ((SL + 1) % 3 == 0)
      gascol = "vCDCH:Gas2";
    else if ((SL + 2) % 3 == 0)
      gascol = "vCDCH:Gas3";

    if (SL % 3 == 0)
      wirecol = "vCDCH:Wire1";
    else if ((SL + 1) % 3 == 0)
      wirecol = "vCDCH:Wire2";
    else if ((SL + 2) % 3 == 0)
      wirecol = "vCDCH:Wire3";

    if (SL == 0) {

      double stereoOut0 = atan(radius_ringOut_0 * (1.0 * dropFactor * epsilonFactor));

      dd4hep::Hyperboloid HypeLayer0(inner_radius_0, 0.0, radius_ringOut_0 - secure, stereoOut0, halflength);
      lvLayerVol.push_back(dd4hep::Volume("lvLayerInit", HypeLayer0, description.material("GasHe_90Isob_10")));
      lvLayerVol.back().setVisAttributes(description, "vCDCH:Pb");

      epsilonInGwRing = atan(inGuardRad * (1.0 + dropFactor) * epsilonFactor);
      zlength = halflength;
      zlength -= sin(epsilonInGwRing) * inGWradii;
      zlength /= cos(epsilonInGwRing);

      ground_wires.mother_volume = lvLayerVol.back();
      ground_wires.type = "G";
      ground_wires.num = nInGWire / 2;
      ground_wires.radius = inGuardRad - inGWradii;
      ground_wires.phi = phi_ring1;
      ground_wires.phioffset = ringangle;
      ground_wires.stereo = epsilonInGwRing;
      ground_wires.halfalpha = halfalpha;
      ground_wires.thickness = 0.5 * InGWireShellThickIn * enlarge;  // half the inner thickness as radius of tube
      ground_wires.halflength = zlength;

      dd4hep::Tube Gwire(0.0, ground_wires.thickness, halflength);
      lvGwireVol.push_back(dd4hep::Volume("Gwire_inner", Gwire, description.material("G4_Al")));
      lvGwireVol.back().setVisAttributes(description, wirecol);

      ground_wires.volume = lvGwireVol.back();
      CDCHBuild::PlaceWires(ground_wires, FWireShellThickOut, halflength, 0, SL, -1);

      ground_wires.radius = inGuardRad + inGWradii + extShiftFW;
      ground_wires.phioffset = ringangle + phi_ring;
      ground_wires.stereo = -1.0 * epsilonInGwRing;
      CDCHBuild::PlaceWires(ground_wires, FWireShellThickOut, halflength, nInGWire / 2, SL, -1);

      drop = radius_ring_0 * dropFactor;
      radius_ring = radius_ring_0 + drop;
      epsilon = atan(radius_ring * epsilonFactor);
      radius_ringIn_0 = radius_ring_0 - FWradii - 2.0 * secure;
      radius_ringIn = radius_ringIn_0 + drop;
      radius_ringOut_0 = radius_ring_0 + FWradii;
      radius_ringOut = radius_ringOut_0 + drop;
      epsilonIn = atan(sqrt(pow(radius_ringIn, 2) - pow(radius_ringIn_0, 2)) / halflength);
      epsilonOut = atan(sqrt(pow(radius_ringOut, 2) - pow(radius_ringOut_0, 2)) / halflength);

      dd4hep::Hyperboloid HypeLayer1(radius_ringIn_0, epsilonIn, radius_ringOut_0, epsilonOut, halflength);
      lvLayerVol.push_back(dd4hep::Volume("lvLayer_0", HypeLayer1, description.material("GasHe_90Isob_10")));
      lvLayerVol.back().setVisAttributes(description, "vCDCH:Plastic");

      zlength = halflength;
      zlength -= sin(epsilon) * FWradii;
      zlength /= cos(epsilon);

      field_wires_top.mother_volume = lvLayerVol.back();
      field_wires_top.type = "F";
      field_wires_top.num = nFWire1;
      field_wires_top.radius = radius_ringIn_0 - FWradii - extShiftFW;
      field_wires_top.phi = phi_ring1;
      field_wires_top.phioffset = ringangle + cellStaggering - phi_ring;
      field_wires_top.stereo = sign_epsilon * epsilon;
      field_wires_top.halfalpha = halfalpha;
      field_wires_top.thickness = 0.5 * FWireShellThickIn * enlarge;
      field_wires_top.halflength = zlength;

      lvFwireName = dd4hep::_toString(SL, "lvFwire_%d_init");

      dd4hep::Tube Fwire(0.0, field_wires_top.thickness, halflength);
      lvFwireVol.push_back(dd4hep::Volume(lvFwireName, Fwire, description.material("G4_Al")));
      lvFwireVol.back().setVisAttributes(description, wirecol);

      field_wires_top.volume = lvFwireVol.back();
      CDCHBuild::PlaceWires(field_wires_top, FWireShellThickOut, halflength, 0, SL, -1);

      radius_ring_0 += FWradii;

    } else {

      delta_radius_ring = 2. * TMath::Pi() * radius_ringOut_0 / (num_wire - TMath::Pi());
    }

    //------------------------------------------------------------------------
    // Starting the layer ("ring") loop. nRing=8
    //------------------------------------------------------------------------

    for (int iring = 0; iring < nRing; iring++) {
      //------------------------------------------------------------------------
      // Fill the geometry parameters of the layer. Each layer lies
      // on top of the following one, so new ringIn = old ringOut
      //------------------------------------------------------------------------

      inscribedRadius = 0.5 * delta_radius_ring;
      circumscribedRadius = inscribedRadius * sqrt(2.0);
      senseWireRing_radius_0 = radius_ring_0 + inscribedRadius;
      sign_epsilon *= -1;

      radius_ringIn_0 = radius_ringOut_0;
      radius_ringIn = radius_ringOut;
      epsilonIn = epsilonOut;

      radius_ringOut_0 = radius_ringIn_0 + FWireDiameter + 2.0 * secure;
      radius_ringOut = radius_ringOut_0 + drop;
      epsilonOut = atan(sqrt(diff_of_squares(radius_ringOut, radius_ringOut_0)) / halflength);

      zlength = halflength;

      // save bottom layer inner radius and epsilon before they are modified to build the whole layer hyperboloid volume 
      radius_ringIn_whole_cell = radius_ringIn_0;
      epsilonIn_whole_cell = epsilonIn;

      //------------------------------------------------------------------------
      // Reduce zlength to avoid volume extrusions and check the staggering
      //------------------------------------------------------------------------

      zlength -= sin(epsilon) * FWradii;
      zlength /= cos(epsilon);

      if (iring % 2 == 1)
        cellStaggering = phi_ring;
      else
        cellStaggering = 0.0;

      //------------------------------------------------------------------------
      // Fill the field wire struct with all the relevant information to be
      // passed to the PlaceWires function. This is the bottom of the cell.
      //------------------------------------------------------------------------

      field_wires_bottom.type = "F";
      field_wires_bottom.num = nFWire1;
      field_wires_bottom.radius = radius_ringIn_0 + FWradii + extShiftFW;
      field_wires_bottom.phi = phi_ring1;
      field_wires_bottom.phioffset = ringangle + cellStaggering;
      field_wires_bottom.stereo = sign_epsilon * epsilon;
      field_wires_bottom.halfalpha = halfalpha;
      field_wires_bottom.thickness = 0.5 * FWireShellThickIn * enlarge;
      field_wires_bottom.halflength = zlength;
      field_wires_bottom.name = "lvWire_SL" + std::to_string(SL) + "_ring" + std::to_string(iring) + "_type" + field_wires_bottom.type + "_stereo" + std::to_string(field_wires_bottom.stereo) + "_bottom";

      //------------------------------------------------------------------------
      // Define the field wire name and build the field wire volume
      //------------------------------------------------------------------------

      lvFwireName = dd4hep::_toString(SL, "lvFwire_%d") + dd4hep::_toString(iring, "_%d");

      dd4hep::Tube Fwire(0.0, field_wires_bottom.thickness, halflength);
      lvFwireVol.push_back(dd4hep::Volume(lvFwireName, Fwire, description.material("G4_Al")));
      lvFwireVol.back().setVisAttributes(description, wirecol);

      //------------------------------------------------------------------------
      // Add the field wire volume to the struct and call the magic function
      //------------------------------------------------------------------------

      field_wires_bottom.volume = lvFwireVol.back();
      apply_wire_coating(field_wires_bottom, FWireShellThickOut, halflength);

      //------------------------------------------------------------------------
      // Next, fill the geometry parameters of the central layer.
      //------------------------------------------------------------------------

      iradius = radius_ring_0;
      radius_ring_0 += delta_radius_ring;
      drop = radius_ring_0 * dropFactor;

      radius_ringIn_0 = radius_ringOut_0;
      radius_ringIn = radius_ringOut;
      epsilonIn = epsilonOut;
      radius_ringOut_0 = radius_ring_0 - FWireDiameter - 2.0 * secure;
      radius_ringOut = radius_ringOut_0 + drop;
      epsilonOut = atan(sqrt(diff_of_squares(radius_ringOut, radius_ringOut_0)) / halflength);
      zlength = halflength;

      //------------------------------------------------------------------------
      // Reduce zlength to avoid volume extrusions
      //------------------------------------------------------------------------

      zlength -= sin(epsilon) * CntFWradii;
      zlength /= cos(epsilon);

      //------------------------------------------------------------------------
      // Fill the sense wire struct with all the relevant information to be
      // passed to the PlaceWires function.
      //------------------------------------------------------------------------

      sense_wires.type = "S";
      sense_wires.num = num_wire;
      sense_wires.radius = senseWireRing_radius_0;
      sense_wires.phi = phi;
      sense_wires.phioffset = cellStaggering;
      sense_wires.stereo = sign_epsilon * epsilon;
      sense_wires.halfalpha = halfalpha;
      sense_wires.thickness = 0.5 * SWireShellThickIn * enlarge;
      sense_wires.halflength = zlength;
      sense_wires.name = "lvWire_SL" + std::to_string(SL) + "_ring" + std::to_string(iring) + "_type" + sense_wires.type + "_stereo" + std::to_string(sense_wires.stereo);

      //------------------------------------------------------------------------
      // Define the sense wire name and build the sense wire volume
      //------------------------------------------------------------------------

      lvSwireName = dd4hep::_toString(SL, "lvSwire_%d") + dd4hep::_toString(iring, "_%d");

      dd4hep::Tube Swire(0.0, sense_wires.thickness, halflength);
      dd4hep::Volume lvSwireVol(lvSwireName, Swire, description.material("G4_W"));
      lvSwireVol.setVisAttributes(description, wirecol);
      sense_wires.volume = lvSwireVol;
      apply_wire_coating(sense_wires, SWireShellThickOut, halflength);

      //------------------------------------------------------------------------
      // Tune the radius and epsilon of the central field wires
      //------------------------------------------------------------------------

      idelta_radius = delta_radius_ring * 0.5;
      iradius += idelta_radius;
      epsilon = atan(iradius * (1.0 + dropFactor) * epsilonFactor);

      //------------------------------------------------------------------------
      // Fill the central field wire struct with all the relevant information
      // and call the magic function.
      //------------------------------------------------------------------------

      field_wires_center.type = "F";
      field_wires_center.num = num_wire;
      field_wires_center.radius = iradius;
      field_wires_center.phi = phi;
      field_wires_center.phioffset = ringangle + cellStaggering;
      field_wires_center.stereo = sign_epsilon * epsilon;
      field_wires_center.halfalpha = halfalpha;
      field_wires_center.thickness = 0.5 * FWireShellThickIn * enlarge;
      field_wires_center.halflength = zlength;
      field_wires_center.volume = lvFwireVol.back();
      field_wires_center.name = "lvWire_SL" + std::to_string(SL) + "_ring" + std::to_string(iring) + "_type" + field_wires_center.type + "_stereo" + std::to_string(field_wires_center.stereo) + "_middle";
      apply_wire_coating(field_wires_center, FWireShellThickOut, halflength);


      //------------------------------------------------------------------------
      // Next, fill the geometry parameters of the upper layer.
      //------------------------------------------------------------------------

      radius_ringIn_0 = radius_ringOut_0;
      radius_ringIn = radius_ringOut;
      epsilonIn = epsilonOut;
      radius_ringOut_0 = radius_ringIn_0 + FWireDiameter + 2.0 * secure;
      radius_ringOut = radius_ringOut_0 + drop;
      epsilonOut = atan(sqrt(diff_of_squares(radius_ringOut, radius_ringOut_0)) / halflength);
      zlength = halflength;

      //------------------------------------------------------------------------
      // Build the hyperboloid shape and the volume of the layer. This is the
      // central layer of the cell.
      //------------------------------------------------------------------------

      //HypeLayer.push_back(dd4hep::Hyperboloid(radius_ringIn_0, epsilonIn, radius_ringOut_0, epsilonOut, zlength));
      //lvLayerVol.push_back(dd4hep::Volume(lvName3, HypeLayer.back(), description.material("GasHe_90Isob_10")));
      //lvLayerVol.back().setVisAttributes(description, gascol);

      // Create hyperboloid volume of the whole ring for cellID definition
      wholeHyperboloidVolumeName = dd4hep::_toString(SL, "hyperboloid_SL_%d") + dd4hep::_toString(iring, "_ring_%d");
      dd4hep::Hyperboloid whole_ring_hyperboloid = dd4hep::Hyperboloid(radius_ringIn_whole_cell, epsilonIn_whole_cell, radius_ringOut_0, epsilonOut, zlength);
      dd4hep::Volume whole_ring_hyperboloid_volume = dd4hep::Volume(wholeHyperboloidVolumeName, whole_ring_hyperboloid, description.material("GasHe_90Isob_10"));
      whole_ring_hyperboloid_volume.setVisAttributes(description, gascol);
      //whole_ring_hyperboloid_volume.setSensitiveDetector(sens_det);
      registerVolume(wholeHyperboloidVolumeName, whole_ring_hyperboloid_volume);
      //cout << "Placing Volume: " << wholeHyperboloidVolumeName << endl;
      //parentVol.placeVolume(volume(wholeHyperboloidVolumeName));
      dd4hep::PlacedVolume whole_ring_hyperboloid_placedVolume;
      whole_ring_hyperboloid_placedVolume = parentVol.placeVolume(whole_ring_hyperboloid_volume);
      CDCHDetector.setPlacement(whole_ring_hyperboloid_placedVolume);
      whole_ring_hyperboloid_placedVolume.addPhysVolID("superLayer", SL).addPhysVolID("ring", iring);

      //------------------------------------------------------------------------
      // Reduce zlength to avoid volume extrusions
      //------------------------------------------------------------------------

      zlength -= sin(epsilon) * FWradii;
      zlength /= cos(epsilon);

      //------------------------------------------------------------------------
      // Fill the field wire struct with all the relevant information and
      // call the magic function. This is the top of the cell.
      //------------------------------------------------------------------------

      field_wires_top.type = "F";
      field_wires_top.num = nFWire1;
      field_wires_top.radius = radius_ringIn_0 - FWradii - extShiftFW;
      field_wires_top.phi = phi_ring1;
      field_wires_top.phioffset = ringangle + cellStaggering;
      field_wires_top.stereo = sign_epsilon * epsilon;
      field_wires_top.halfalpha = halfalpha;
      field_wires_top.thickness = 0.5 * FWireShellThickIn * enlarge;
      field_wires_top.halflength = zlength;
      field_wires_top.volume = lvFwireVol.back();
      field_wires_top.name = "lvWire_SL" + std::to_string(SL) + "_ring" + std::to_string(iring) + "_type" + field_wires_top.type + "_stereo" + std::to_string(field_wires_top.stereo) + "_top";
      apply_wire_coating(field_wires_top, FWireShellThickOut, halflength);

      //if(setWireSensitive){
      //  field_wires_bottom.volume.setSensitiveDetector(sens_det);
      //  field_wires_center.volume.setSensitiveDetector(sens_det);
      //  sense_wires.volume.setSensitiveDetector(sens_det);
      //  field_wires_top.volume.setSensitiveDetector(sens_det);
      //}

      // arbitrarily extended tube section to build the sensitive volume ID associated to the wire from boolean operation with the layer hyperboloid
      dd4hep::Tube cellID_tube_for_boolean(0, sense_wires.radius * 2, halflength, - sense_wires.phi / 2.0, sense_wires.phi / 2.0);
      dd4hep::IntersectionSolid cellID_shape;
      dd4hep::PlacedVolume cellID_placedvolume;
      string cellID_volume_name;
      dd4hep::Volume cellID_volume;
      dd4hep::PlacedVolume field_wire_bottom_placedvolume;
      dd4hep::PlacedVolume sense_wire_placedvolume;
      dd4hep::PlacedVolume field_wire_center_placedvolume;
      dd4hep::PlacedVolume field_wire_top_placedvolume;
      
      // Radial translation 
      dd4hep::Translation3D radial_translation_sense_wire(sense_wires.radius, 0., 0.);
      // stereo rotation
      dd4hep::RotationX rot_stereo_sense_wire(sense_wires.stereo);
      // extract the number of wire ratio to place field wires in the loop for sense wires
      // it is not very elegant but the sense wire define the sensitive volume in which wires are placed 
      float middle_to_middle_num_wire_ratio = field_wires_center.num/float(sense_wires.num);
      float middle_to_bottom_num_wire_ratio = field_wires_bottom.num/float(sense_wires.num);
      float middle_to_top_num_wire_ratio = field_wires_top.num/float(sense_wires.num);
      if(ceilf(middle_to_middle_num_wire_ratio) != middle_to_middle_num_wire_ratio || ceilf(middle_to_bottom_num_wire_ratio) != middle_to_bottom_num_wire_ratio || ceilf(middle_to_top_num_wire_ratio) != middle_to_top_num_wire_ratio)
          throw std::runtime_error("Error: Failed to build CDCH. Please make sure that the number of wires in top/center cell rings is always a multiple of the number of wires in the middle of the cell");
      // loop to arrange the wires in phi, starting with the sense wires to be able to build the volume associated to the cell
      for (int phi_index = 0; phi_index < sense_wires.num; phi_index++) {
        // Prepare the cell sensitive volume as the intersection of the hyperboloid and a rotated tube segment
        // phi rotation
        dd4hep::RotationZ iRot(sense_wires.phioffset + sense_wires.phi * phi_index);
        // complete transformation for the sense wires
        dd4hep::Transform3D total_transformation(iRot * radial_translation_sense_wire * rot_stereo_sense_wire);
        // create the intersection of the tube with the hyperboloid after rotating the tube in phi and stereo angle 
        cellID_shape = dd4hep::IntersectionSolid(whole_ring_hyperboloid, cellID_tube_for_boolean, dd4hep::Transform3D(dd4hep::RotationZ(sense_wires.phioffset + sense_wires.phi * phi_index) * dd4hep::RotationX(sense_wires.stereo)));
        cellID_volume_name = dd4hep::_toString(SL, "cellIDvolume_SL_%d") + dd4hep::_toString(iring, "_ring_%d") + dd4hep::_toString(phi_index, "_phi_%d");
        cellID_volume = dd4hep::Volume(cellID_volume_name, cellID_shape, description.material("GasHe_90Isob_10"));
        cellID_volume.setVisAttributes(description, gascol);
        cellID_volume.setSensitiveDetector(sens_det);
        cellID_placedvolume = whole_ring_hyperboloid_volume.placeVolume(cellID_volume);
        cellID_placedvolume.addPhysVolID("phi", phi_index).addPhysVolID("hitorigin", 0).addPhysVolID("stereo", sense_wires.stereo > 0 ? 0 : 1).addPhysVolID("layerInCell", 0);

        // place the wires. The transformation is: apply the stereo angle rotation, translate the wire to the required radius, apply the phi rotation
        // sense wires in the radial middle of the cell
        sense_wire_placedvolume = cellID_volume.placeVolume(sense_wires.volume, total_transformation);

        // bottom field wires
        for(int sub_phi_index = phi_index * middle_to_bottom_num_wire_ratio; sub_phi_index < (phi_index * middle_to_bottom_num_wire_ratio) + middle_to_bottom_num_wire_ratio; sub_phi_index++){
          field_wire_bottom_placedvolume = cellID_volume.placeVolume(field_wires_bottom.volume, dd4hep::Transform3D(dd4hep::RotationZ(field_wires_bottom.phioffset + field_wires_bottom.phi * sub_phi_index) * dd4hep::Translation3D(field_wires_bottom.radius, 0., 0.) *dd4hep::RotationX(field_wires_bottom.stereo)));
          //if(setWireSensitive)
          //  field_wire_bottom_placedvolume.addPhysVolID("phi", sub_phi_index).addPhysVolID("hitorigin", 2).addPhysVolID("stereo", field_wires_center.stereo > 0 ? 0 : 1).addPhysVolID("layerInCell", 1);
        }

        // central field wires
        for(int sub_phi_index = phi_index * middle_to_middle_num_wire_ratio; sub_phi_index < (phi_index * middle_to_middle_num_wire_ratio) + middle_to_middle_num_wire_ratio; sub_phi_index++){
          field_wire_center_placedvolume = cellID_volume.placeVolume(field_wires_center.volume, dd4hep::Transform3D(dd4hep::RotationZ(field_wires_center.phioffset + field_wires_center.phi * sub_phi_index) * dd4hep::Translation3D(field_wires_center.radius, 0., 0.) *dd4hep::RotationX(field_wires_center.stereo)));
          //if(setWireSensitive)
          //  field_wire_center_placedvolume.addPhysVolID("phi", sub_phi_index).addPhysVolID("hitorigin", 2).addPhysVolID("stereo", field_wires_center.stereo > 0 ? 0 : 1).addPhysVolID("layerInCell", 2);
        }

        // top field wires
        for(int sub_phi_index = phi_index * middle_to_top_num_wire_ratio; sub_phi_index < (phi_index * middle_to_top_num_wire_ratio) + middle_to_top_num_wire_ratio; sub_phi_index++){
          field_wire_top_placedvolume = cellID_volume.placeVolume(field_wires_top.volume, dd4hep::Transform3D(dd4hep::RotationZ(field_wires_top.phioffset + field_wires_top.phi * sub_phi_index) * dd4hep::Translation3D(field_wires_top.radius, 0., 0.) *dd4hep::RotationX(field_wires_top.stereo)));
          //if(setWireSensitive)
          //  field_wire_top_placedvolume.addPhysVolID("phi", sub_phi_index).addPhysVolID("hitorigin", 2).addPhysVolID("stereo", field_wires_center.stereo > 0 ? 0 : 1).addPhysVolID("layerInCell", 3);
        }
      }

      //CDCHBuild::PlaceWires(field_wires_bottom, FWireShellThickOut, halflength, 0, SL, iring);
      //CDCHBuild::PlaceWires(sense_wires, FWireShellThickOut, halflength, 0, SL, iring, sens_det, parentVol);
      //CDCHBuild::PlaceWires(field_wires_center, FWireShellThickOut, halflength, 0, SL, iring);
      //CDCHBuild::PlaceWires(field_wires_top, FWireShellThickOut, halflength, 0, SL, iring);

      //------------------------------------------------------------------------
      // Scale the delta radius of the ring for next iteration
      //------------------------------------------------------------------------

      delta_radius_ring *= scaleFactor;
    }

    if (SL == (nSuperLayer - 1)) {

      radius_ringIn_0 = radius_ringOut_0;
      radius_ringIn = radius_ringOut;
      epsilonIn = epsilonOut;
      radius_ringOut_0 = radius_ring_0 + FWireDiameter + 2.0 * secure;
      radius_ringOut = radius_ringOut_0 + drop;
      epsilonOut = atan(sqrt(diff_of_squares(radius_ringOut, radius_ringOut_0)) / halflength);

      dd4hep::Hyperboloid HypeLayerOut(radius_ringIn_0, epsilonIn, radius_ringOut_0, epsilonOut, halflength);
      lvLayerVol.push_back(dd4hep::Volume("lvLayerOut", HypeLayerOut, description.material("GasHe_90Isob_10")));
      lvLayerVol.back().setVisAttributes(description, "vCDCH:Plastic");

      zlength = halflength;
      zlength -= sin(epsilon) * FWradii;
      zlength /= cos(epsilon);

      field_wires_bottom.mother_volume = lvLayerVol.back();
      field_wires_bottom.type = "F";
      field_wires_bottom.num = nFWire1;
      field_wires_bottom.radius = radius_ringIn_0 + FWradii + extShiftFW;
      field_wires_bottom.phi = phi_ring1;
      field_wires_bottom.phioffset = ringangle + cellStaggering + phi_ring;
      field_wires_bottom.stereo = -1. * sign_epsilon * epsilon;
      field_wires_bottom.halfalpha = halfalpha;
      field_wires_bottom.thickness = 0.5 * FWireShellThickIn * enlarge;
      field_wires_bottom.halflength = zlength;

      lvFwireName = dd4hep::_toString(SL, "lvFwire_%d_out");

      dd4hep::Tube Fwire(0.0, field_wires_bottom.thickness, halflength);
      lvFwireVol.push_back(dd4hep::Volume(lvFwireName, Fwire, description.material("G4_Al")));
      lvFwireVol.back().setVisAttributes(description, wirecol);

      field_wires_bottom.volume = lvFwireVol.back();
      CDCHBuild::PlaceWires(field_wires_bottom, FWireShellThickOut, halflength, 0, SL, -1);

      //------------------------------------------------------------------------
      // Start placing the outer layer of guard wires
      //------------------------------------------------------------------------

      radius_ringIn_0 = radius_ringOut_0;
      radius_ringIn = radius_ringOut;
      epsilonIn = epsilonOut;
      radius_ringOut_0 = radius_ring_0 + FWireDiameter + 2.0 * secure;
      radius_ringOut = radius_ringOut_0 + drop;
      epsilonOut = atan(sqrt(diff_of_squares(radius_ringOut, radius_ringOut_0)) / halflength);

      dd4hep::Hyperboloid HypeLayerOutG(radius_ringIn_0, epsilonOut, outer_radius - envelop_Outer_thickness - 0.0001,
                                        0.0, halflength);
      lvLayerVol.push_back(dd4hep::Volume("lvLayerOutG", HypeLayerOutG, description.material("GasHe_90Isob_10")));
      lvLayerVol.back().setVisAttributes(description, "vCDCH:Pb");

      epsilonOutGwRing = atan(outGuardRad * (1.0 + dropFactor) * epsilonFactor);
      zlength = halflength;
      zlength -= sin(epsilonOutGwRing) * inGWradii;
      zlength /= cos(epsilonOutGwRing);

      ground_wires.mother_volume = lvLayerVol.back();
      ground_wires.type = "G";
      ground_wires.num = nOutGWire / 2;
      ground_wires.radius = outGuardRad - inGWradii;
      ground_wires.phi = phi_ring1;
      ground_wires.phioffset = ringangle;
      ground_wires.stereo = epsilonOutGwRing;
      ground_wires.halfalpha = halfalpha;
      ground_wires.thickness = 0.5 * OutGWireShellThickIn * enlarge;
      ground_wires.halflength = zlength;

      dd4hep::Tube Gwire(0.0, ground_wires.thickness, halflength);
      lvGwireVol.push_back(dd4hep::Volume("Gwire_outer", Gwire, description.material("G4_Al")));
      lvGwireVol.back().setVisAttributes(description, wirecol);

      ground_wires.volume = lvGwireVol.back();
      CDCHBuild::PlaceWires(ground_wires, FWireShellThickOut, halflength, 0, SL, -1);

      ground_wires.radius = outGuardRad + inGWradii + extShiftFW;
      ground_wires.phioffset = ringangle + phi_ring;
      ground_wires.stereo = -1.0 * epsilonOutGwRing;
      CDCHBuild::PlaceWires(ground_wires, FWireShellThickOut, halflength, nOutGWire / 2, SL, -1);
    }
  }


  Int_t sizeLayer = lvLayerVol.size();

  for (Int_t i = 0; i < sizeLayer; i++) {
    registerVolume(lvLayerVol.at(i).name(), lvLayerVol.at(i));
    //cout << "Placing Volume: " << lvLayerVol.at(i).name() << endl;
    //pv = parentVol.placeVolume(volume(lvLayerVol.at(i).name()));
    //CDCHDetector.setPlacement(pv);
    parentVol.placeVolume(volume(lvLayerVol.at(i).name()));
  }

  double PosEndcapGas = halflength + 0.5 * GasEndcapWallThick;
  double PosEndcapCopper = halflength + GasEndcapWallThick + 0.5 * CopperEndcapWallThick;
  double PosEndcapKapton = halflength + GasEndcapWallThick + CopperEndcapWallThick + 0.5 * KaptonEndcapWallThick;
  double PosEndcapCarbon =
      halflength + GasEndcapWallThick + CopperEndcapWallThick + KaptonEndcapWallThick + 0.5 * CarbonEndcapWallThick;

  parentVol.placeVolume(lvInnerWallCarbon);
  parentVol.placeVolume(lvInnerWallCopper);
  parentVol.placeVolume(lvInnerWallGas);
  parentVol.placeVolume(lvOuterWallCarbon1);
  parentVol.placeVolume(lvOuterWallCarbon2);
  parentVol.placeVolume(lvOuterWallCopper);
  parentVol.placeVolume(lvOuterWallFoam);
  parentVol.placeVolume(lvEndcapWallGas, dd4hep::Position(0., 0., PosEndcapGas));
  parentVol.placeVolume(lvEndcapWallCopper, dd4hep::Position(0., 0., PosEndcapCopper));
  parentVol.placeVolume(lvEndcapWallKapton, dd4hep::Position(0., 0., PosEndcapKapton));
  parentVol.placeVolume(lvEndcapWallCarbon, dd4hep::Position(0., 0., PosEndcapCarbon));
  parentVol.placeVolume(lvEndcapWallGas, dd4hep::Position(0., 0., -PosEndcapGas));
  parentVol.placeVolume(lvEndcapWallCopper, dd4hep::Position(0., 0., -PosEndcapCopper));
  parentVol.placeVolume(lvEndcapWallKapton, dd4hep::Position(0., 0., -PosEndcapKapton));
  parentVol.placeVolume(lvEndcapWallCarbon, dd4hep::Position(0., 0., -PosEndcapCarbon));
}
} //namespace

static dd4hep::Ref_t create_element(dd4hep::Detector& description, xml_h e, dd4hep::SensitiveDetector sens_det) {

  xml_det_t x_det = e;
  CDCHBuild builder(description, x_det, sens_det);
  string det_name = x_det.nameStr();

  dd4hep::printout(dd4hep::DEBUG, "CreateCDCH", "Detector name: %s with ID: %s", det_name.c_str(), x_det.id());

  DetElement CDCH_det = builder.detector;  // ( det_name, x_det.id() );
  dd4hep::Box CDCH_box("5000/2", "5000/2", "5000/2");

  Volume envelope("lvCDCH", CDCH_box, description.air());
  envelope.setVisAttributes(description, "vCDCH:Air");
  PlacedVolume pv;

  dd4hep::printout(dd4hep::DEBUG, "CreateCDCH", "MotherVolume is: %s", envelope.name());
  sens_det.setType("tracker");

  builder.buildVolumes(e);
  builder.placeDaughters(CDCH_det, envelope, e);

  // ******************************************************
  // Build CDCH cable
  // ******************************************************

  builder.build_layer(CDCH_det, envelope, sens_det);

  // ******************************************************
  // Build CDCH cell and beam plug
  // ******************************************************

  //  builder.build_cell();
  //  builder.build_beamplug();

  // ******************************************************
  // Assemble CDCH
  // ******************************************************

  //  builder.build_CDCH( Ecal_det, envelope );

  // ******************************************************
  // Place the CDCH in the world
  // ******************************************************

  pv = builder.placeDetector(envelope);
  pv.addPhysVolID("system", x_det.id());

  return CDCH_det;
}

DECLARE_DETELEMENT(DriftChamber_o1_v01, create_element)
