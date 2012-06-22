#include "simple_physics_tracker.h"
#include "sparse_utils.h"
#include "config_tracking.h"
#include "algorithm_common.h"
#include "plotting_tracking.h"
#include "utils/conversions.h"
#include <fstream>

using namespace Eigen;
using namespace std;

SimplePhysicsTracker::SimplePhysicsTracker(TrackedObject::Ptr obj, VisibilityInterface* visInt, Environment::Ptr env) :
  m_obj(obj),
  m_visInt(visInt),
  m_env(env),
  m_obsPlot(new PointCloudPlot(3)),
  m_obsColorPlot(new PointCloudPlot(6)),
  m_obsDebugPlot(new PointCloudPlot(6)),
  m_estPlot(new PlotSpheres()),
  m_corrPlot(new PlotLines()),
  m_enableObsPlot(false),
  m_enableObsColorPlot(false),
  m_enableObsDebugPlot(false),
  m_enableEstPlot(false),
  m_enableCorrPlot(false)
{
	m_env->add(m_obsPlot);
	m_env->add(m_obsColorPlot);
	m_env->add(m_obsDebugPlot);
	m_env->add(m_estPlot);
	m_env->add(m_corrPlot);
	m_corrPlot->setDefaultColor(1,1,0,.3);
}

void SimplePhysicsTracker::updateInput(ColorCloudPtr obsPts) {
  m_obsPts = toBulletVectors(obsPts);
  M_obsPts = toEigenMatrix(obsPts);
}

void SimplePhysicsTracker::doIteration() {
  VectorXf vis = m_visInt->checkNodeVisibility(m_obj);
  m_estPts = m_obj->getPoints();
  SparseMatrixf corr;
  m_stdev = (.03*METERS)*VectorXf::Ones(m_obj->m_nNodes);
  MatrixXf obsPtsEigen = toEigenMatrix(m_obsPts);

  M_estPts = m_obj->getFeatures();
  MatrixXf node_stdev(1,6);
  node_stdev << (.03*METERS), (.03*METERS), (.03*METERS), 0.6*255.0, 0.6*255.0, 0.6*255.0;
  M_stdev = node_stdev.replicate(m_obj->m_nNodes, 1);

  // E STEP
  //estimateCorrespondence(toEigenMatrix(m_estPts), (VectorXf) m_stdev.array().square(), vis, obsPtsEigen, TrackingConfig::outlierParam, corr);
  //estimateCorrespondence(M_estPts.block(0,0,M_estPts.rows(),3), (VectorXf) m_stdev.array().square(), vis, M_obsPts.block(0,0,M_obsPts.rows(),3), TrackingConfig::outlierParam, corr);
  //estimateCorrespondence(M_estPts.block(0,0,M_estPts.rows(),4), (MatrixXf) M_stdev.block(0,0,M_stdev.rows(),4), vis, M_obsPts.block(0,0,M_obsPts.rows(),4), TrackingConfig::outlierParam, corr);
  estimateCorrespondence(M_estPts, M_stdev, vis, M_obsPts, TrackingConfig::outlierParam, corr);

  VectorXf inlierFrac = colSums(corr);

//  if (m_enableObsPlot) plotObs(m_obsPts, inlierFrac, m_obsPlot);
//  else m_obsPlot->clear();
//  if (m_enableEstPlot) plotNodesAsSpheres(m_estPts, vis, m_stdev.array(), m_estPlot);
//  else m_estPlot->clear();
//  if (m_enableCorrPlot) drawCorrLines(m_corrPlot, m_estPts, m_obsPts, corr);
//  else m_corrPlot->clear();

  if (m_enableObsPlot) plotObs(toBulletVectors(M_obsPts.block(0,0,M_obsPts.rows(),3)), inlierFrac, m_obsPlot);
	else m_obsPlot->clear();
	if (m_enableObsColorPlot) plotObs(M_obsPts, m_obsColorPlot);
	else m_obsColorPlot->clear();
	if (m_enableObsDebugPlot) plotObs(M_obsDebug, m_obsDebugPlot);
	else m_obsDebugPlot->clear();
	if (m_enableEstPlot) plotNodesAsSpheres(M_estPts, vis, M_stdev, m_estPlot);
	else m_estPlot->clear();
	if (m_enableCorrPlot) drawCorrLines(m_corrPlot, toBulletVectors(M_estPts.block(0,0,M_estPts.rows(),3)), toBulletVectors(M_obsPts.block(0,0,M_obsPts.rows(),3)), corr);
	else m_corrPlot->clear();

  // M STEP
  m_obj->applyEvidence(corr, obsPtsEigen);
  //m_obj->applyEvidence(corr, M_obsPts);
  m_env->step(.03,2,.015);
}
