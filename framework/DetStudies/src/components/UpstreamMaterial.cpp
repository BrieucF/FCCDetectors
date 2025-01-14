#include "UpstreamMaterial.h"

#include "k4Interface/IGeoSvc.h"

// datamodel
#include "edm4hep/CalorimeterHitCollection.h"
#include "edm4hep/MCParticleCollection.h"

#include "CLHEP/Vector/ThreeVector.h"
#include "GaudiKernel/ITHistSvc.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TVector2.h"

// DD4hep
#include "DD4hep/Detector.h"
#include "DD4hep/Readout.h"

DECLARE_COMPONENT(UpstreamMaterial)

UpstreamMaterial::UpstreamMaterial(const std::string& aName, ISvcLocator* aSvcLoc) : GaudiAlgorithm(aName, aSvcLoc), m_geoSvc("GeoSvc", aName) {
  declareProperty("deposits", m_deposits, "Energy deposits (input)");
  declareProperty("particle", m_particle, "Generated single-particle event (input)");
}
UpstreamMaterial::~UpstreamMaterial() {}

StatusCode UpstreamMaterial::initialize() {
  if (GaudiAlgorithm::initialize().isFailure()) return StatusCode::FAILURE;
  
  if (!m_geoSvc) {
    error() << "Unable to locate Geometry Service. "
            << "Make sure you have GeoSvc and SimSvc in the right order in the configuration." << endmsg;
    return StatusCode::FAILURE;
  }
  // check if readouts exist
  if (m_geoSvc->lcdd()->readouts().find(m_readoutName) == m_geoSvc->lcdd()->readouts().end()) {
    error() << "Readout <<" << m_readoutName << ">> does not exist." << endmsg;
    return StatusCode::FAILURE;
  }
  m_histSvc = service("THistSvc");
  if (!m_histSvc) {
    error() << "Unable to locate Histogram Service" << endmsg;
    return StatusCode::FAILURE;
  }
  for (uint i = 0; i < m_numLayers; i++) {
    m_cellEnergyPhi.push_back(new TH1F(("upstreamEnergy_phi" + std::to_string(i)).c_str(),
                                       ("Energy deposited in layer " + std::to_string(i)).c_str(), 1000, -m_phi,
                                       m_phi));
    if (m_histSvc->regHist("/det/upstreamEnergy_phi" + std::to_string(i), m_cellEnergyPhi.back()).isFailure()) {
      error() << "Couldn't register histogram" << endmsg;
      return StatusCode::FAILURE;
    }
    m_upstreamEnergyCellEnergy.push_back(
        new TH2F(("upstreamEnergy_presamplerEnergy" + std::to_string(i)).c_str(),
                 ("Upstream energy vs energy deposited in layer " + std::to_string(i)).c_str(), 4000, 0, m_energy, 4000,
                 0, m_energy));
    if (m_histSvc
            ->regHist("/det/upstreamEnergy_presamplerEnergy" + std::to_string(i), m_upstreamEnergyCellEnergy.back())
            .isFailure()) {
      error() << "Couldn't register hist" << endmsg;
      return StatusCode::FAILURE;
    }
  }
  return StatusCode::SUCCESS;
}

StatusCode UpstreamMaterial::execute() {
  auto decoder = m_geoSvc->lcdd()->readout(m_readoutName).idSpec().decoder();
  double sumEupstream = 0.;
  std::vector<double> sumEcells;
  sumEcells.assign(m_numLayers, 0);

  // first check MC phi angle
  const auto particle = m_particle.get();
  double phi = 0;
  for (const auto& part : *particle) {
    auto mom = part.getMomentum();
    phi = atan2(mom.y, mom.x);
  }

  // get the energy deposited in the cryostat and in the detector (each layer)
  const auto deposits = m_deposits.get();
  for (const auto& hit : *deposits) {
    dd4hep::DDSegmentation::CellID cID = hit.getCellID();
    const auto& field = "cryo";
    int id = decoder->get(cID, field);
    if (id == 0) {
      sumEcells[id - m_firstLayerId] += hit.getEnergy();
    } else {
      sumEupstream += hit.getEnergy();
    }
  }
  for (uint i = 0; i < m_numLayers; i++) {
    // calibrate the energy in the detector
    sumEcells[i] /= m_samplingFraction[i];
    m_cellEnergyPhi[i]->Fill(phi, sumEcells[i]);
    m_upstreamEnergyCellEnergy[i]->Fill(sumEcells[i], sumEupstream);
    verbose() << "Energy deposited in layer " << i << " = " << sumEcells[i]
              << "\t energy deposited in the cryostat = " << sumEupstream << endmsg;
  }
  return StatusCode::SUCCESS;
}

StatusCode UpstreamMaterial::finalize() { return GaudiAlgorithm::finalize(); }
