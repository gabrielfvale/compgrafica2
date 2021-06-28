#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct Photon
{
  float pos[3];             // photon position
  float power[3];           // photon power (uncompressed)
  short plane;              // splitting plane for kd-tree
  unsigned char theta, phi; // incoming direction
} Photon;

typedef struct NearestPhotons
{
  int max;
  int found;
  int got_heap;
  float pos[3];
  float *dist2;
  const Photon **index;
} NearestPhotons;

class PhotonMap
{
public:
  PhotonMap(int max_phot);
  ~PhotonMap();
  void store(const float power[3], const float pos[3], const float dir[3]);
  void scale_photon_power(const float scale);
  void balance(void);
  void irradiance_estimate(float irrad[3], const float pos[3], const float normal[3], const float max_dist, const int nphotons) const;
  void locate_photons(NearestPhotons *const np, const int index) const;
  void photon_dir(float *dir, const Photon *p) const;
  int stored_photons;
  int max_photons;

private:
  void balance_segment(Photon **pbal, Photon **porg, const int index, const int start, const int end);
  void median_split(Photon **p, const int start, const int end, const int median, const int axis);

  Photon *photons;

  int half_stored_photons;
  int prev_scale;

  float costheta[256];
  float sintheta[256];
  float cosphi[256];
  float sinphi[256];

  float bbox_min[3];
  float bbox_max[3];
};
