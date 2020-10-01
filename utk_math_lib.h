

#ifndef UTK_BV_UTK_MATH_LIB_H
#define UTK_BV_UTK_MATH_LIB_H
#include "osqp.h"
#include "utk_vector.h"
//template<typename V, typename VV, typename A>
//double if_r(const V& tmpT, const VV& PG, vector<float> &w,
//                         const A& pdt, int dimen, int k, int rou);

template<typename V, typename VV>
double inflection_radius(const V &tmpT, const VV &PG, vector<c_float> &w,
                         int pdt_idx, int dimen, int k);

template<typename V, typename VV, typename A>
double inflection_radius(const V &fetch, const VV &PG, vector<c_float> &w,
                         const A &pdt, int dimen, int k);

template<typename V, typename VV, typename A>
double inflection_radius(const V &fetch, const VV &PG, vector<c_float> &w,
                         const A &pdt, int dimen, int k, const double &rou,
                         multiset<double> &radius, unsigned int &cnt);

template<typename V>
double domin_r_ij(const V &w, const V &h_ij);

template<typename V>
bool dominate(const V &v1, const V &v2);

template<typename VV>
vector<int> k_skyband(VV &P, const int &k);

template<typename A, typename B>
double inflection_radius(vector<c_float> &w,
                         const A &the, const B &cmp, int dimen, int k);

template<typename V, typename VV, typename A>
double inflection_radius(const V &fetch, const VV &PG, vector<c_float> &w,
                         const A &pdt, int dimen, int k, const double &rou);

template<typename V, typename U>
bool v1_dominate_v2(const V &v1, const U &v2, size_t size);

template<typename V>
double domin_r_ij2(const V &w, const V &h_ij);


template<typename V>
inline V proj(const V &u, const V &v);

template<typename VV>
VV gram_schmidt_process(const VV &input);

template<typename V>
inline c_float L2_norm(V &v);

template<typename INTEGER>
vector<vector<c_float>> gen_r_domain_basevec(INTEGER dim);

template<typename VV, typename V>
void gen_r_domain_vec_dfs(V &&cur, int next,
                          VV &ret, VV &e);

template<typename INTEGER>
vector<vector<c_float>> gen_r_domain_vec(INTEGER dim);

template<typename V>
double domin_r_ij3(const V &w, const V &h_ij);

template<typename V, typename VV, typename FLOAT>
bool v2_r_dominate_v1(const V &v1, const V &v2, const V &w, const VV &r_domain_vec, const FLOAT &rho);




template<typename VF, typename VI, typename VV, typename FLOAT>
bool isR_skyband(const VV &PG, const VI&vs, const VF &opt, const VF &w, const FLOAT &rho, int k);


#include "utk_math_lib_impl.h"
//the implementation (definition) of a template class
// or function must be in the same file as its declaration.

// the template fun and non-template fun are not suggested to be compile together
#endif //UTK_BV_UTK_MATH_LIB_H
