#include "FusionEKF.h"
#include <iostream>
#include "Eigen/Dense"
#include "tools.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::cout;
using std::endl;
using std::vector;

/**
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);
  
  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;
  //measurement matrix
  
  H_laser_ << 1.0, 0.0, 0.0, 0.0,
             0.0, 1.0, 0.0, 0.0;
  
  // state covariance matrix
  ekf_.P_ = MatrixXd(4,4);
  ekf_.P_ << 1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 100, 0 ,
             0, 0, 0 , 100;
  
  
  
  //setting acceleration noise components

   noise_ax = 10.0;
   noise_ay = 10.0;

}

/**
 * Destructor.
 */
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
  /**
   * Initialization
   */
  if (!is_initialized_) {
   
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1,1,1,1;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      double rho = measurement_pack.raw_measurements_[0];
      double theta = measurement_pack.raw_measurements_[1];
      double rate = measurement_pack.raw_measurements_[2];
      while (theta > M_PI) theta -= 2.0 * M_PI;
      while (theta < -M_PI) theta += 2.0 * M_PI;
      double dx = rho*cos(theta);
      double dy = rho*sin(theta);
      double vx = rate*cos(theta);
      double vy = rate*sin(theta);
      ekf_.x_ << dx, dy, vx, vy ;
     
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      
      ekf_.x_ << measurement_pack.raw_measurements_[0],
                 measurement_pack.raw_measurements_[1],
                 0,
                 0;
    }
    
    
    //ekf_.Init(x, P, F, H_laser_, R_laser_, R_radar_ ,Q);
    previous_timestamp_ = measurement_pack.timestamp_;
    is_initialized_ = true;
    return;
  }

  double dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;
  previous_timestamp_ = measurement_pack.timestamp_;
  
  //initial transition matrix
  ekf_.F_   = MatrixXd(4,4);
  ekf_.F_ << 1, 0, dt, 0,
             0, 1, 0, dt,
             0, 0, 1, 0,
             0, 0, 0, 1;
  double dt_2 = dt * dt;
  double dt_3 = dt_2 * dt;
  double dt_4 = dt_3 * dt;

  // Modify the F matrix so that the time is integrated
  ekf_.F_(0, 2) = dt;
  ekf_.F_(1, 3) = dt;

  // set the process covariance matrix Q
  ekf_.Q_ = MatrixXd(4, 4);
  ekf_.Q_ <<  dt_4/4*noise_ax, 0, dt_3/2*noise_ax, 0,
              0, dt_4/4*noise_ay, 0, dt_3/2*noise_ay,
              dt_3/2*noise_ax, 0, dt_2*noise_ax, 0,
              0, dt_3/2*noise_ay, 0, dt_2*noise_ay;

  
  /**
   * Prediction
   */

  ekf_.Predict();

  /**
   * Update
   */
  
  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar update
    ekf_.H_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser update
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
     ekf_.Update(measurement_pack.raw_measurements_);

  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}