#include "madgwickFilter.h"

struct quaternion q_est = { 1, 0, 0, 0};       // initialize with as unit vector with real component  = 1
struct quaternion quat_mult (struct quaternion L, struct quaternion R){
    
    
    struct quaternion product;
    product.q1 = (L.q1 * R.q1) - (L.q2 * R.q2) - (L.q3 * R.q3) - (L.q4 * R.q4);
    product.q2 = (L.q1 * R.q2) + (L.q2 * R.q1) + (L.q3 * R.q4) - (L.q4 * R.q3);
    product.q3 = (L.q1 * R.q3) - (L.q2 * R.q4) + (L.q3 * R.q1) + (L.q4 * R.q2);
    product.q4 = (L.q1 * R.q4) + (L.q2 * R.q3) - (L.q3 * R.q2) + (L.q4 * R.q1);
    
    return product;
}

// Updates q_est. gyro angular velocity components are in rad/s, accelerometer components are normalized
void madgwick_update(float ax, float ay, float az, float gx, float gy, float gz){
    //Variables and constants
    struct quaternion q_est_prev = q_est;
    struct quaternion q_est_dot = {0};           
    const struct quaternion q_g_ref = {0, 0, 0, 1};
    struct quaternion q_a = {0, ax, ay, az};    // raw acceleration values (not normalized)
    float F_g [3] = {0};                        // objective function for gravity
    float J_g [3][4] = {0};                     // jacobian matrix
    struct quaternion gradient = {0};           // gradient 
    
    // Attitude estimate from gyro with numerical integration 
    struct quaternion q_w;                   // places gyroscope readings in a quaternion
    q_w.q1 = 0;                              // real component 0
    q_w.q2 = gx;
    q_w.q3 = gy;
    q_w.q4 = gz;
    quat_scalar(&q_w, 0.5);                  
    q_w = quat_mult(q_est_prev, q_w);        // dq/dt = (1/2)q*w

    //Attitude estimate from acc with gradient descent 
    quat_Normalization(&q_a);              // normalize acc quaternion
    F_g[0] = 2*(q_est_prev.q2 * q_est_prev.q4 - q_est_prev.q1 * q_est_prev.q3) - q_a.q2;    //Compute objective function for gravity
    F_g[1] = 2*(q_est_prev.q1 * q_est_prev.q2 + q_est_prev.q3* q_est_prev.q4) - q_a.q3;
    F_g[2] = 2*(0.5 - q_est_prev.q2 * q_est_prev.q2 - q_est_prev.q3 * q_est_prev.q3) - q_a.q4;
    
    //Compute jacobian matrix
    J_g[0][0] = -2 * q_est_prev.q3;
    J_g[0][1] = 2 * q_est_prev.q4;
    J_g[0][2] = -2 * q_est_prev.q1;
    J_g[0][3] = 2 * q_est_prev.q2;
    J_g[1][0] = 2 * q_est_prev.q2;
    J_g[1][1] = 2 * q_est_prev.q1;
    J_g[1][2] = 2 * q_est_prev.q4;
    J_g[1][3] = 2 * q_est_prev.q3;
    J_g[2][0] = 0;
    J_g[2][1] = -4 * q_est_prev.q2;
    J_g[2][2] = -4 * q_est_prev.q3;
    J_g[2][3] = 0;
    
    // compute gradient = J_g'*F_g
    gradient.q1 = J_g[0][0] * F_g[0] + J_g[1][0] * F_g[1] + J_g[2][0] * F_g[2];
    gradient.q2 = J_g[0][1] * F_g[0] + J_g[1][1] * F_g[1] + J_g[2][1] * F_g[2];
    gradient.q3 = J_g[0][2] * F_g[0] + J_g[1][2] * F_g[1] + J_g[2][2] * F_g[2];
    gradient.q4 = J_g[0][3] * F_g[0] + J_g[1][3] * F_g[1] + J_g[2][3] * F_g[2];
    
    // gradient normalization
    quat_Normalization(&gradient);
  
    //sensor fusion
    quat_scalar(&gradient, BETA);             // multiply normalized gradient by beta
    quat_sub(&q_est_dot, q_w, gradient);        // subtract above from q_w, the integrated gyro quaternion
    quat_scalar(&q_est_dot, DELTA_T);
    quat_add(&q_est, q_est_prev, q_est_dot);     // Integrate orientation rate to find position
    quat_Normalization(&q_est);                 // normalize the orientation of the estimate
}

//returns as pointers, roll pitch and yaw from q, using right hand system (Roll,x,phi)(Pitch,y,theta)(Yaw,z,psi) 
void eulerAngles(struct quaternion q, float* roll, float* pitch, float* yaw){
    
    *yaw = atan2f((2*q.q2*q.q3 - 2*q.q1*q.q4), (2*q.q1*q.q1 + 2*q.q2*q.q2 -1));  
    *pitch = -asinf(2*q.q2*q.q4 + 2*q.q1*q.q3);                                  
    *roll  = atan2f((2*q.q3*q.q4 - 2*q.q1*q.q2), (2*q.q1*q.q1 + 2*q.q4*q.q4 -1));
    
    *yaw *= (180.0f / PI);
    *pitch *= (180.0f / PI);
    *roll *= (180.0f / PI);

}