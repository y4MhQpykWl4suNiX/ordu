
#include "case_study.h"
#include "rtree.h"
#include "skyline.h"
#include "utk_math_lib.h"
double jaccard(set<int> &a, set<int>&b){
    // jaccard similarity
    set<int> all(a.begin(), a.end());
    for(int bi:b){
        all.insert(bi);
    }
    double cnt=0;
    for(int bi:b){
        if(a.find(bi)!=a.end()){
            cnt++;
        }
    }
    return cnt/all.size();
}

double jaccard(vector<int>::iterator ab, vector<int>::iterator ae, vector<int>::iterator bb, vector<int>::iterator be){
    // jaccard similarity
    set<int> a(ab, ae);
    set<int> b(bb, be);
    return jaccard(a, b);
}


double precision(vector<int>::iterator gtb, vector<int>::iterator gte, vector<int>::iterator rb, vector<int>::iterator re){
    set<int> gt(gtb, gte);
    set<int> r(rb, re);
    return precision(gt, r);
}

double precision(set<int> &groundTruth, set<int>&result){
    // TP, true positive
    // FP, false positive
    // precision, TP/(TP+FP)
    int TP=0;
    for(int r:result){
        if(groundTruth.find(r)!=groundTruth.end()){
            ++TP;
        }
    }
//    int FP=result.size()-TP;
    return (double)TP/result.size();
}

double recall(vector<int>::iterator gtb, vector<int>::iterator gte, vector<int>::iterator rb, vector<int>::iterator re){
    set<int> gt(gtb, gte);
    set<int> r(rb, re);
    return recall(gt, r);
}

double recall(set<int> &groundTruth, set<int>&result){
    // TP, true positive
    // FN, false negative
    // precision, TP/(TP+FN)
    int TP=0;
    for(int r:result){
        if(groundTruth.find(r)!=groundTruth.end()){
            ++TP;
        }
    }
    return (double)TP/groundTruth.size();
}


int total_do_cnt(map<int, unordered_set<int>> &do_id, vector<int> &test_this){
    unordered_set<int> s;
    for (int i:test_this) {
        for (int j: do_id.find(i)->second) {
            s.insert(j);
        }
    }
    return s.size();
}

vector<int> OSS_skyline(int objCnt, int r, Rtree*rtree, float **PointSet, int dim){
    vector<long> one_skyband;
//            kskyband(dim, *rtree, one_skyband, PointSet, k);
    kskyband(dim, *rtree, one_skyband, PointSet, 1);

    map<int, unordered_set<int>> do_id;
    for (int id:one_skyband) {
        do_id[id]=unordered_set<int>();
    }
    for (int id: one_skyband) {
        for (int i = 1; i <= objCnt; ++i) {
            if(i==id){
                continue;
            }
            if(v1_dominate_v2(PointSet[id], PointSet[i], dim)){
                do_id[id].insert(i);
            }
        }
    }

    int n=one_skyband.size();

    std::vector<bool> v(n);
    std::fill(v.begin(), v.begin() + r, true);

    int max_tdo=0;
    vector<int> best_cb;
    do {
        vector<int> one_cb;
        for (int i = 0; i < n; ++i) {
            if (v[i]) {
                one_cb.push_back(one_skyband[i]);
            }
        }
        int cur=total_do_cnt(do_id, one_cb);
        if(cur>max_tdo){
            max_tdo=cur;
            best_cb=one_cb;
        }
    } while (std::prev_permutation(v.begin(), v.end()));
    return best_cb;
}