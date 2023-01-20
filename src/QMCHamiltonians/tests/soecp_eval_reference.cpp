#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>
#include <cassert>

double PI = 4. * std::atan(1);

//Same quadrature grid as in QMCPACK
std::vector<std::vector<double>> quad = {{1, 0, 0},
                                         {-1, 1.224646853e-16, 0},
                                         {0.4472135901, 0.8944271803, 0},
                                         {-0.4472135901, 0.7236068249, 0.5257310867},
                                         {0.4472135901, 0.2763932049, 0.8506507874},
                                         {-0.4472135901, -0.2763932049, 0.8506507874},
                                         {0.4472135901, -0.7236068249, 0.5257310867},
                                         {-0.4472135901, -0.8944271803, 1.095357398e-16},
                                         {0.4472135901, -0.7236068249, -0.5257310867},
                                         {-0.4472135901, -0.2763932049, -0.8506507874},
                                         {0.4472135901, 0.2763932049, -0.8506507874},
                                         {-0.4472135901, 0.7236068249, -0.5257310867}};
std::vector<double> wt = {0.08333333582, 0.08333333582, 0.08333333582, 0.08333333582, 0.08333333582, 0.08333333582,
                          0.08333333582, 0.08333333582, 0.08333333582, 0.08333333582, 0.08333333582, 0.08333333582};

//add simple Bspline Jastrow to compare against
double B(double x, int k, int i, const std::vector<double>& t)
{
  double c1, c2;
  if (k == 0)
    return (x >= t[i] && x < t[i + 1]) ? 1.0 : 0.0;
  if (t[i + k] == t[i])
    c1 = 0.0;
  else
    c1 = (x - t[i]) / (t[i + k] - t[i]) * B(x, k - 1, i, t);
  if (t[i + k + 1] == t[i + 1])
    c2 = 0.0;
  else
    c2 = (t[i + k + 1] - x) / (t[i + k + 1] - t[i + 1]) * B(x, k - 1, i + 1, t);
  return c1 + c2;
}

double bspline(double x, const std::vector<double>& t, const std::vector<double>& c, int k)
{
  int n = t.size() - k - 1;
  assert(n >= k + 1);
  assert(n <= c.size());
  double val = 0.0;
  for (int i = 0; i < n; i++)
    val += c[i] * B(x, k, i, t);
  return val;
}

class cubicBSpline
{
public:
  cubicBSpline(double rcut, double cusp_val, const std::vector<double>& coeffs)
  {
    int numknots = coeffs.size();
    coeffs_.resize(numknots + 6);
    knots_.resize(numknots + 6);

    double delta = rcut / (numknots + 1);
    coeffs_[0]   = -2 * cusp_val * delta + coeffs[1];
    coeffs_[1]   = coeffs[0];
    coeffs_[2]   = coeffs[1];
    std::copy(coeffs.begin() + 2, coeffs.end(), coeffs_.begin() + 3);
    for (int i = 0; i < knots_.size(); i++)
      knots_[i] = (i - 3) * delta;
  }
  double getVal(double x) const { return bspline(x, knots_, coeffs_, degree); }

private:
  std::vector<double> coeffs_;
  std::vector<double> knots_;
  const int degree = 3;
};

void testJastrow()
{
  //testing 1b cusp from gen_bspline_jastrow.py
  {
    const double rcut = 10;
    const double cusp = 2.0;

    std::vector<double> coeffs = {-0.2032153051,  -0.1625595974,  -0.143124599,   -0.1216434956,
                                  -0.09919771951, -0.07111729038, -0.04445345869, -0.02135082917};

    cubicBSpline spl(rcut, cusp, coeffs);

    //from gen_bspline_jastrow.py
    std::vector<double> refVals = {-0.9304041433,   -0.252599792,   -0.1637586749,  -0.1506226948,  -0.1394848415,
                                   -0.128023472,    -0.1161729491,  -0.1036884223,  -0.08992443283, -0.07519614609,
                                   -0.06054074137,  -0.04654631918, -0.03347994129, -0.0211986378,  -0.01004416026,
                                   -0.002594125744, -0.000166024047};

    std::vector<double> rvals(refVals.size());
    for (int i = 0; i < refVals.size(); i++)
      rvals[i] = i * 0.60;

    bool good = true;
    for (int i = 0; i < refVals.size(); i++)
      if (std::abs(refVals[i] - spl.getVal(rvals[i])) > 1E-6)
      {
        std::cout << "error: " << rvals[i] << " " << refVals[i] << " != " << spl.getVal(rvals[i]) << std::endl;
        good = false;
      }
    std::cout << "1b jastrow w/ cusp correct: " << good << std::endl;
  }

  //test J2 from gen_bspline_jastrow.py
  {
    const double rcut = 10;
    const double cusp = -0.5;

    std::vector<double> coeffs = {0.02904699284, -0.1004179,    -0.1752703883, -0.2232576505, -0.2728029201,
                                  -0.3253286875, -0.3624525145, -0.3958223107, -0.4268582166, -0.4394531176};

    cubicBSpline spl(rcut, cusp, coeffs);

    //from gen_bspline_jastrow.py
    std::vector<double> refVals = {0.1374071801,   -0.04952403966, -0.121361995,  -0.1695590431, -0.2058414025,
                                   -0.2382237097,  -0.2712606182,  -0.3047843679, -0.3347515004, -0.3597048574,
                                   -0.3823503292,  -0.4036800017,  -0.4219818468, -0.4192355508, -0.3019238309,
                                   -0.09726352421, -0.006239062395};


    double r   = 0.6935647049;
    double val = spl.getVal(r);
    std::cout << r << " " << val << std::endl;
    std::vector<double> rvals(refVals.size());
    for (int i = 0; i < refVals.size(); i++)
      rvals[i] = i * 0.60;

    bool good = true;
    for (int i = 0; i < refVals.size(); i++)
      if (std::abs(refVals[i] - spl.getVal(rvals[i])) > 1E-6)
      {
        std::cout << "error: " << rvals[i] << " " << refVals[i] << " != " << spl.getVal(rvals[i]) << std::endl;
        good = false;
      }
    std::cout << "2b jastrow correct: " << good << std::endl;
  }
}

std::complex<double> I = std::complex<double>(0, 1);

std::complex<double> Ylm(int l, int m, const std::vector<double>& sph)
{
  //From wiki
  double pref;
  switch (l)
  {
  case 0:
    return std::complex<double>(0.5 * std::sqrt(1. / PI), 0.0);
    break;
  case 1:
    switch (m)
    {
    case -1:
      pref = 0.5 * std::sqrt(3. / (2 * PI));
      return pref * std::exp(-I * sph[2]) * std::sin(sph[1]);
      break;
    case 0:
      pref = 0.5 * std::sqrt(3. / (PI));
      return pref * std::cos(sph[1]);
      break;
    case 1:
      pref = -0.5 * std::sqrt(3. / (2 * PI));
      return pref * std::exp(I * sph[2]) * std::sin(sph[1]);
      break;
    default:
      exit(1);
      break;
    }
    break;
  case 2:
    switch (m)
    {
    case -2:
      pref = 0.25 * std::sqrt(15. / (2 * PI));
      return pref * std::exp(-2.0 * I * sph[2]) * std::sin(sph[1]) * std::sin(sph[1]);
      break;
    case -1:
      pref = 0.5 * std::sqrt(15. / (2 * PI));
      return pref * std::exp(-I * sph[2]) * std::sin(sph[1]) * std::cos(sph[1]);
      break;
    case 0:
      pref = 0.25 * std::sqrt(5. / (PI));
      return pref * (3. * std::cos(sph[1]) * std::cos(sph[1]) - 1.);
      break;

    case 1:
      pref = -0.5 * std::sqrt(15. / (2 * PI));
      return pref * std::exp(I * sph[2]) * std::sin(sph[1]) * std::cos(sph[1]);
      break;
    case 2:
      pref = 0.25 * std::sqrt(15. / (2 * PI));
      return pref * std::exp(2.0 * I * sph[2]) * std::sin(sph[1]) * std::sin(sph[1]);
      break;
    }
  case 3:
    switch (m)
    {
    case -3:
      pref = 0.125 * std::sqrt(35. / PI);
      return pref * std::exp(-3. * I * sph[2]) * std::pow(std::sin(sph[1]), 3);
      break;
    case -2:
      pref = 0.25 * std::sqrt(105. / (2 * PI));
      return pref * std::exp(-2. * I * sph[2]) * std::pow(std::sin(sph[1]), 2) * std::cos(sph[1]);
      break;
    case -1:
      pref = 0.125 * std::sqrt(21. / PI);
      return pref * std::exp(-I * sph[2]) * std::sin(sph[1]) * (5 * std::pow(std::cos(sph[1]), 2) - 1);
      break;
    case 0:
      pref = 0.25 * std::sqrt(7. / PI);
      return pref * (5 * std::pow(std::cos(sph[1]), 3) - 3. * std::cos(sph[1]));
      break;
    case 1:
      pref = -0.125 * std::sqrt(21. / PI);
      return pref * std::exp(I * sph[2]) * std::sin(sph[1]) * (5 * std::pow(std::cos(sph[1]), 2) - 1);
      break;
    case 2:
      pref = 0.25 * std::sqrt(105. / (2 * PI));
      return pref * std::exp(2. * I * sph[2]) * std::pow(std::sin(sph[1]), 2) * std::cos(sph[1]);
      break;
    case 3:
      pref = -0.125 * std::sqrt(35. / PI);
      return pref * std::exp(3. * I * sph[2]) * std::pow(std::sin(sph[1]), 3);
      break;
    }
    break;
  default:
    exit(1);
    break;
  }
}


std::vector<double> cart2sph(const std::vector<double>& cart)
{
  //Physics convention
  std::vector<double> sph(3);
  sph[0] = std::sqrt(cart[0] * cart[0] + cart[1] * cart[1] + cart[2] * cart[2]);
  sph[1] = std::acos(cart[2] / sph[0]);
  sph[2] = std::atan2(cart[1], cart[0]);
  return sph;
}

std::vector<double> sph2cart(const std::vector<double>& sph)
{
  //Physics convention
  std::vector<double> cart(3);
  cart[0] = sph[0] * std::sin(sph[1]) * std::cos(sph[2]);
  cart[1] = sph[0] * std::sin(sph[1]) * std::sin(sph[2]);
  cart[2] = sph[0] * std::cos(sph[1]);
  return cart;
}

//spin representation
std::complex<double> chiu(double s) { return std::exp(I * s); }
std::complex<double> chid(double s) { return std::exp(-I * s); }

//simple plane-wave orbitals for spinor
std::complex<double> orbu(const std::vector<double>& sph)
{
  std::vector<double> cart = sph2cart(sph);
  return std::exp(I * (cart[0] + cart[1] + cart[2]));
}
std::complex<double> orbd(const std::vector<double>& sph)
{
  std::vector<double> cart = sph2cart(sph);
  return std::exp(2. * I * (cart[0] + cart[1] + cart[2]));
}

//single particle spinors
std::complex<double> spinor0(const std::vector<double>& sph, double s)
{
  return orbu(sph) * chiu(s) + orbd(sph) * chid(s);
}
std::complex<double> spinor1(const std::vector<double>& sph, double s)
{
  return orbd(sph) * chiu(s) + orbu(sph) * chid(s);
}

// <s1 | s_d | s2>, for L.S operator in SOECP
std::complex<double> sMatrixElement(double s1, double s2, int d)
{
  switch (d)
  {
  case 0:
    return std::complex<double>(std::cos(s1 + s2), 0.0);
    break;
  case 1:
    return std::complex<double>(std::sin(s1 + s2), 0.0);
    break;
  case 2:
    return std::complex<double>(0.0, std::sin(s1 - s2));
    break;
  default:
    exit(1);
    break;
  }
}

int kroneckerDelta(int x, int y) { return (x == y) ? 1 : 0; }

//<l m1 | l_d | l m2>
std::complex<double> lMatrixElement(int l, int m1, int m2, int d)
{
  double pref1, pref2, val;
  switch (d)
  {
  case 0:
    pref1 = std::sqrt(l * (l + 1) - m2 * (m2 + 1));
    pref2 = std::sqrt(l * (l + 1) - m2 * (m2 - 1));
    val   = 0.5 * (pref1 * kroneckerDelta(m1, m2 + 1) + pref2 * kroneckerDelta(m1, m2 - 1));
    return std::complex<double>(val, 0.0);
    break;
  case 1:
    pref1 = std::sqrt(l * (l + 1) - m2 * (m2 - 1));
    pref2 = std::sqrt(l * (l + 1) - m2 * (m2 + 1));
    val   = 0.5 * (pref1 * kroneckerDelta(m1, m2 - 1) - pref2 * kroneckerDelta(m1, m2 + 1));
    return std::complex<double>(0.0, val);
    break;
  case 2:
    return std::complex<double>(m2 * kroneckerDelta(m1, m2), 0.0);
    break;
  default:
    exit(1);
    break;
  }
}

//simple radial dependence. coded in so_ecp_test.xml
//used in so_ecp_test.xml for each spin channel
double Wso(int l, double r) { return exp(-l * r * r); }

std::complex<double> TWF(std::vector<std::vector<double>>& positions, std::vector<double>& spins, cubicBSpline& J2)
{
  std::vector<double> cart0 = sph2cart(positions[0]);
  std::vector<double> cart1 = sph2cart(positions[1]);
  double dx                 = (cart1[0] - cart0[0]);
  double dy                 = (cart1[1] - cart0[1]);
  double dz                 = (cart1[2] - cart0[2]);
  double r12                = std::sqrt(dx * dx + dy * dy + dz * dz);
  std::complex<double> det  = spinor0(positions[0], spins[0]) * spinor1(positions[1], spins[1]) -
      spinor1(positions[0], spins[0]) * spinor0(positions[1], spins[1]);
  return det * exp(-J2.getVal(r12));
}

std::complex<double> calcAngInt(int iel,
                                double s2,
                                std::vector<std::vector<double>>& positions,
                                std::vector<double>& spins,
                                cubicBSpline& J2)
{
  std::complex<double> angint(0.0, 0.0);
  for (int i = 0; i < quad.size(); i++)
  {
    std::vector<double> sph2 = cart2sph(quad[i]);
    sph2[0] *= positions[iel][0]; //now scaled to appropriate distance

    std::vector<std::vector<double>> newpositions = positions;
    std::vector<double> newspins                  = spins;
    newpositions[iel]                             = sph2;
    newspins[iel]                                 = s2;

    std::complex<double> integrand(0.0, 0.0);
    for (int l = 1; l <= 3; l++)
    {
      std::complex<double> msum(0.0, 0.0);
      for (int m1 = -l; m1 <= l; m1++)
      {
        for (int m2 = -l; m2 <= l; m2++)
        {
          std::complex<double> ldots(0.0, 0.0);
          for (int d = 0; d < 3; d++)
            ldots += lMatrixElement(l, m1, m2, d) * sMatrixElement(spins[iel], newspins[iel], d);
          msum += Ylm(l, m1, positions[iel]) * std::conj(Ylm(l, m2, newpositions[iel])) * ldots;
        }
      }
      integrand += Wso(l, positions[iel][0]) * msum;
    }

    std::complex<double> num   = TWF(newpositions, newspins, J2);
    std::complex<double> denom = TWF(positions, spins, J2);
    integrand *= num / denom;
    angint += integrand * wt[i] * 4.0 * PI;
  }
  return angint;
}

std::complex<double> calcVal(int npts,
                             int iel,
                             std::vector<std::vector<double>>& positions,
                             std::vector<double>& spins,
                             cubicBSpline& J2)
{
  std::complex<double> sint(0.0, 0.0);
  double smin = 0.0;
  double smax = 2 * PI;
  double h    = (smax - smin) / npts;
  for (int k = 1; k <= npts - 1; k += 2)
  {
    double s2                   = smin + k * h;
    std::complex<double> angint = calcAngInt(iel, s2, positions, spins, J2);
    sint += 4 * h / 3. * angint;
  }
  for (int k = 2; k <= npts - 2; k += 2)
  {
    double s2                   = smin + k * h;
    std::complex<double> angint = calcAngInt(iel, s2, positions, spins, J2);
    sint += 2 * h / 3. * angint;
  }
  sint += h / 3. * calcAngInt(iel, smin, positions, spins, J2);
  sint += h / 3. * calcAngInt(iel, smax, positions, spins, J2);
  sint /= (2.0 * PI);

  return sint;
}

void calcSOECP(int npts)
{
  //2el system, atom at origin, 2body jastrow only
  std::vector<double> pos_e1                 = {0.138, -0.24, 0.216};
  std::vector<double> pos_e2                 = {-0.216, 0.24, -0.138};
  std::vector<std::vector<double>> positions = {cart2sph(pos_e1), cart2sph(pos_e2)};
  double spin_e1                             = 0.0;
  double spin_e2                             = 0.51;
  std::vector<double> spins                  = {spin_e1, spin_e2};

  double rcut                = 10.0;
  double cusp                = -0.25;
  std::vector<double> coeffs = {0.02904699284, -0.1004179,    -0.1752703883, -0.2232576505, -0.2728029201,
                                -0.3253286875, -0.3624525145, -0.3958223107, -0.4268582166, -0.4394531176};
  cubicBSpline J2(rcut, cusp, coeffs);

  std::complex<double> val;
  for (int iel = 0; iel < 2; iel++)
    val += calcVal(npts, iel, positions, spins, J2);
  std::cout << npts << " " << std::setprecision(10) << std::real(val) << " " << std::imag(val) << std::endl;
}


int main()
{
  testJastrow();
  for (int n = 2; n <= 100; n += 2)
    calcSOECP(n);
}
