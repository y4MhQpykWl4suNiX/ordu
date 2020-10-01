#include "iPref.h"
#include "qp_solver.h"
#include "utk_math_lib.h"
#include "qhull_user.h"
#include "lp_user.h"
#include <chrono>
#include "qp_solver2.h"

extern int objCnt;

extern qp_solver qp;

inline double max(double a, float b){
    return a>b?a:b;
}

inline double max(float b, double a){
    return a>b?a:b;
}

vector<int> computeTopK(const int dim, float* PG[], vector<int> &skyband, vector<float>& weight, int k)
{
    multimap<float, int> heap;

    for (int i = 0; i < skyband.size(); i++)
    {
        float score = 0;
        for (int d = 0; d < dim; d++)
        {
            score += (PG[skyband[i]][d] + PG[skyband[i]][d + dim]) / 2*weight[d];
        }

        if (heap.size() < k)
        {
            heap.emplace(score, skyband[i]);
        }
        else if (heap.size() == k && heap.begin()->first < score)
        {
            heap.erase(heap.begin());
            heap.emplace(score, skyband[i]);
        }
    }

    vector<int> topkRet;
    for (auto heapIter = heap.rbegin(); heapIter != heap.rend(); ++heapIter)
    {
        topkRet.push_back(heapIter->second);
    }
    return topkRet;
}

vector<long int> computeTopK(const int dim, float* PG[], vector<long int> &skyband, vector<float>& weight, int k)
{
	multimap<float, long int> heap;

	for (int i = 0; i < skyband.size(); i++)
	{
		float score = 0;
		for (int d = 0; d < dim; d++)
		{
			score += (PG[skyband[i]][d] + PG[skyband[i]][d + dim]) / 2*weight[d];
		}

		if (heap.size() < k)
		{
			heap.emplace(score, skyband[i]);
		}
		else if (heap.size() == k && heap.begin()->first < score)
		{
			heap.erase(heap.begin());
			heap.emplace(score, skyband[i]);
		}
	}

	vector<long int> topkRet;
	for (auto heapIter = heap.rbegin(); heapIter != heap.rend(); ++heapIter)
	{
		topkRet.push_back(heapIter->second);
	}
	return topkRet;
}

bool IsPjdominatePi(const int dimen, float* PG[], long int pi, long int pj)
{
	float pid;
	float pjd;
	int count = 0;
	for (int d = 0; d < dimen; d++)
	{
		pid = (PG[pi][d] + PG[pi][dimen + d]) / 2.0;
		pjd = (PG[pj][d] + PG[pj][dimen + d]) / 2.0;
		if (pjd >= pid)
		{
            count++;
        }else{
            break;
		}
	}


	if (count == dimen)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool incomparableset(float* PG[], long int pi, long int pj, vector<float>& weight)
{
	int dimen = weight.size();
	float spi = 0, spj = 0;
	int cpos = 0;
	int cneg = 0;

	float piv[MAXDIMEN];
	float pjv[MAXDIMEN];
	for (int d = 0; d < dimen; d++)
	{
		piv[d] = (PG[pi][d] + PG[pi][dimen + d]) / 2.0;
		pjv[d] = (PG[pj][d] + PG[pj][dimen + d]) / 2.0;
	}

	for (int i = 0; i < dimen; i++)
	{
		spi += piv[i] * weight[i];
		spj += pjv[i] * weight[i];

		if (piv[i] <= pjv[i])
		{
			cneg++;
		}
		else
		{
			cpos++;
		}
	}

	if (spj >= spi && cneg != 0 && cpos != 0)
		return true;
	else
		return false;
}

vector<float> computePairHP(const int dimen, float* PG[], long int pi, long int pj)
{
	vector<float> retHS(dimen);
    for (int i = 0; i < dimen; ++i) {
        retHS[i]=PG[pi][i]-PG[pj][i];
    }
	return retHS;
}

float computeDis(const vector<float> &tmpHS, const vector<float> &userpref)
{
    float ret=qp.update_w_h_solve(userpref, tmpHS);
    return ret;
}

bool sortbysec(const pair<long int, float> &a, const pair<long int, float> &b)
{
	return (a.second < b.second);
}

float computeradius(const int k, const int dim, long int pi, vector<float>& userpref, vector<long int>& incompSet, float* PG[], float rho)
{
	multiset<float> radiusSKI;
	vector<float> tmpHS(dim);
	float tmpDis;
	for (int ri = 0; ri < incompSet.size(); ri++)
	{
		if (IsPjdominatePi(dim, PG, pi, incompSet[ri]))  // for these dominators, we directly input as FLTMAX
		{
			radiusSKI.insert(INFINITY);
		}
		else
		{
			tmpHS = computePairHP(dim, PG, pi, incompSet[ri]);
			tmpDis = computeDis(tmpHS, userpref);
			radiusSKI.insert(tmpDis);
		}
		if(radiusSKI.size()>k){
		    radiusSKI.erase(radiusSKI.begin());
		    if(*radiusSKI.begin()>=rho){
		        // the default rho is INF
		        // if current radius is larger than rho, than prune it
		        break;
		    }
		}
	}
	if(radiusSKI.size()>=k){
        return *radiusSKI.begin();
    }else{
	    return 0;
	}
}

int countDominator(Rtree& a_rtree, float* PG[], Point& a_pt)
{
    // USED
	queue<long int> H;
	float rd[MAXDIMEN];
	int dimen = a_rtree.m_dimen;
	int NoOfDominators = 0;
	RtreeNode* node;
	RtreeNodeEntry* e0;
	long int NegPageid, pageid;

	float cl[MAXDIMEN], cu[MAXDIMEN];
	for (int d = 0; d < dimen; d++)
	{
		cl[d] = 0;
		cu[d] = a_pt[d];
	}
	Hypercube Dominee_hc(dimen, cl, cu);
	for (int d = 0; d < dimen; d++)
	{
		cl[d] = a_pt[d];
		cu[d] = 1;
	}
	Hypercube Dominator_hc(dimen, cl, cu);

	H.push(a_rtree.m_memory.m_rootPageID);
	while (!H.empty())
	{
		pageid = H.front();
		H.pop();
		node = ramTree[pageid];
		if (node->isLeaf())
		{
			for (int i = 0; i < node->m_usedspace; i++)
			{
				for (int d = 0; d < dimen; d++)
					rd[d] = (PG[node->m_entry[i]->m_id][d] + PG[node->m_entry[i]->m_id][dimen + d]) / 2.0;
				Point tmpPt(dimen, rd);
				if (tmpPt == a_pt)
					continue;
				else if (Dominator_hc.enclose(tmpPt) == true)   //current point lies in the dominator window
				{
					NoOfDominators++;
				}
			}
		}
		else
		{
			e0 = node->genNodeEntry();
			if (Dominator_hc.enclose(e0->m_hc) == true)
			{
				for (int i = 0; i < node->m_usedspace; i++)
					NoOfDominators += node->m_entry[i]->num_records;
			}
			else if (Dominee_hc.enclose(e0->m_hc) == false)
			{
				for (int i = 0; i < node->m_usedspace; i++)
					H.push(node->m_entry[i]->m_id);
			}
		}
	}
	return NoOfDominators;
}

int countNodeDominator(const int dim, const float* pt, vector<long int>& incompSet, float* PG[]) // I am not sure it is correct if we only consider incompSet.
{
	int cnt = 0;
	for (int i = 0; i < incompSet.size(); i++)
	{
		bool isDominator = true;
		for (int di = 0; di < dim; di++)
		{
			if ((PG[incompSet[i]][di] + PG[incompSet[i]][dim + di]) / 2.0 < pt[di])
			{
				isDominator = false;
				break;
			}
		}
		if (isDominator == true)
		{
			cnt++;
		}
	}
	return cnt;
}

bool v2_r_dominate_v1_float(const float* v1, const float* v2, const vector<float> &w, const vector<vector<c_float>> &r_domain_vec, const float &rho) {
    for (const vector<c_float> &v:r_domain_vec) {
//        vector<float> tmp_w = w + rho * v;
//        if (v1 * tmp_w < v2 * tmp_w) {
//            return false;
//        }
        double atc_rho=rho;
        for (int i = 0; i < v.size(); ++i) {
            if(v[i]<0){
                atc_rho=min(atc_rho, -w[i]/v[i]); // in case of w[i] + \rho * v[i] <0 or >1
            }
        }
        vector<float> tmp_w(w.size());
        for (int i = 0; i < tmp_w.size(); ++i) {
            tmp_w[i]=w[i]+atc_rho*v[i];
        }
        float s1=0, s2=0;
        for (int i = 0; i < tmp_w.size(); ++i) {
            s1+=tmp_w[i]*v1[i];
        }
        for (int i = 0; i < tmp_w.size(); ++i) {
            s2+=tmp_w[i]*v2[i];
        }
        if(s1>s2){
            return false;
        }
    }
    return true;
}

int countNodeRDominator(const int dim, float* pt, const float radius,
        vector<float>& userpref, vector<long int>& incompSet, float* PG[])
{
    int r_dominate_cnt=0;
    for (const long int&v:incompSet) {
        if(v2_r_dominate_v1_float(pt, PG[v], userpref, g_r_domain_vec, radius)){
            ++r_dominate_cnt;
        }
    }
    return r_dominate_cnt;
}

bool r_dominatedByK(const int dim, float* pt, const float radius,
                        vector<float>& userpref, vector<long int>& incompSet, float* PG[], int k)
{
    int r_dominate_cnt=0;
    for (const long int&v:incompSet) {
        if(v2_r_dominate_v1_float(pt, PG[v], userpref, g_r_domain_vec, radius)){
            ++r_dominate_cnt;
            if(r_dominate_cnt>=k){
                break;
            }
        }
    }
    return r_dominate_cnt>=k;
}

float computeRho(const int dimen, const int k, const int X, vector<float>& userpref, Rtree& a_rtree, float* PG[],
        vector<pair<long int, float>> &interval, float radius)
{
	vector<long int> incompSet;
	pair<float, int> candidateOpt;

	RtreeNode* node;
	multimap<float, int, greater<float>> heap;
    multimap<float, int, greater<float>> candidateRet;

	float pt[MAXDIMEN];
	int pageID;
	float tmpScore; // compute the score of option or mbr e w.r.t. userpref
	float tmpRadius; // the inflection radius of point Pi.

	heap.emplace(INFINITY, a_rtree.m_memory.m_rootPageID);

    while (!heap.empty())
	{
		tmpScore = heap.begin()->first;
		pageID = heap.begin()->second;
		heap.erase(heap.begin());
		if (pageID > MAXPAGEID)  // option processing
		{
			if (interval.size() < k)  // Phase I
			{
				interval.emplace_back(pageID - MAXPAGEID, 0);
                incompSet.push_back(pageID - MAXPAGEID);
            }
			else
			{
				if (candidateRet.size() < X - k)  // Phase II
				{
					tmpRadius = computeradius(k, dimen, pageID - MAXPAGEID, userpref, incompSet, PG, radius);
					if(tmpRadius!=INFINITY){
                        candidateRet.emplace(tmpRadius, pageID-MAXPAGEID);
                        if(candidateRet.size()==X-k)
                        {
                            radius = min(radius, candidateRet.begin()->first);
                        }
					}
                }
				else if(X<=k)
				{
                    assert(X==k);// if fails there is a problem in data
                    radius=0;
				    break;
				}
				else   // Phase III
				{
					tmpRadius = computeradius(k, dimen, pageID - MAXPAGEID, userpref, incompSet, PG, candidateRet.begin()->first);
					if (tmpRadius < candidateRet.begin()->first)
					{
						candidateRet.emplace(tmpRadius, pageID - MAXPAGEID);
						candidateRet.erase(candidateRet.begin());
                        candidateOpt = *candidateRet.begin();
                        radius = min(candidateOpt.first, radius);
                    }
				}
				if(tmpRadius!=INFINITY){
                    incompSet.push_back(pageID - MAXPAGEID);
                }
			}
//			incompSet.push_back(pageID - MAXPAGEID);

        }
		else // internal and leaf nodes processing
		{
			node = ramTree[pageID];
			if (node->isLeaf())
			{	
				for (int i = 0; i < node->m_usedspace; i++)
				{
					tmpScore = 0;
					for (int j = 0; j < dimen; j++)
					{
						pt[j] = node->m_entry[i]->m_hc.getCenter()[j];
						tmpScore += pt[j] * userpref[j];
					}
					Point a_pt(dimen, pt);
					if (radius == INFINITY)
					{
                        if (!dominatedByK(dimen, pt, incompSet, PG, k))
						{
							heap.emplace(tmpScore, node->m_entry[i]->m_id + MAXPAGEID);
						}
					}
					else
					{
                        if (!r_dominatedByK(dimen, pt, radius, userpref, incompSet, PG, k))
						{
                            heap.emplace(tmpScore, node->m_entry[i]->m_id + MAXPAGEID);
                        }
					}
				}
			}
			else
			{
				for (int i = 0; i < node->m_usedspace; i++)
				{
					tmpScore = 0;
					for (int j = 0; j < dimen; j++)
					{
						pt[j] = node->m_entry[i]->m_hc.getUpper()[j];
						tmpScore += pt[j] * userpref[j];
					}
					Point a_pt(dimen, pt);
                    if (radius == INFINITY)
                    {
                        if (!dominatedByK(dimen, pt, incompSet, PG, k))
                        {
                            heap.emplace(tmpScore, node->m_entry[i]->m_id);
                        }
                    }
                    else
                    {
                        if (!r_dominatedByK(dimen, pt, radius, userpref, incompSet, PG, k))
                        {
                            heap.emplace(tmpScore, node->m_entry[i]->m_id);
                        }
                    }
				}
			}
		}
	}
    for (auto riter=candidateRet.rbegin();riter!=candidateRet.rend();++riter) {
        interval.emplace_back(riter->second, riter->first);
    }
	if(interval.size()<X){
	    return INFINITY;
	}else{
        return radius;
    }
}

unknown_X_node::unknown_X_node(int d,  const float *dt, int pid){
        dim=d;
        data=dt;
        page_id=pid;
        fetched=false;
        tau=0;// used in unknown efficient
}

float unknown_X_node::radius_LB(){
    return *topK_dominate_radius.begin();
}

void unknown_X_node::init_radius(vector<long int> &fetched_options, float **PointSet, vector<float> &w, int k, float rho){
    for (int i = 0; i < k; ++i) {
        update_radius(PointSet[fetched_options[i]], w);
    }
    for (int j = k; j < fetched_options.size(); ++j) {
        if(this->radius_LB()>=rho){
            break;
        }
        update_radius_erase(PointSet[fetched_options[j]], w);
    }
}

inline bool unknown_X_node::update(int need_to_update) const{// used in unknown efficient
    return this->tau>=need_to_update;
}

void unknown_X_node::update_radius(vector<long int>::iterator begin, vector<long int>::iterator end, float **PointSet, vector<float> &w, float rho){
    for (; begin!=end; ++begin) {
        update_radius_erase(PointSet[*begin], w);
        if(this->radius_LB()>=rho){ // lazy update // used in unknown efficient
            return;
        }
    }
}

void unknown_X_node::update_radius(const float* other_option, vector<float> &w){
    ++tau;// used in unknown efficient
    if(v1_dominate_v2(other_option, data, dim)){
        topK_dominate_radius.insert(INFINITY);
    }else{
        vector<float> tmpHS(data, data+dim);
        for (int i = 0; i < dim; ++i) {
            tmpHS[i]-=other_option[i];
        }
        float tmpDis = computeDis(tmpHS, w);
        topK_dominate_radius.insert(tmpDis);
    }
}

void unknown_X_node::update_radius_erase(const float* other_option, vector<float> &w){
    update_radius(other_option, w);
    topK_dominate_radius.erase(topK_dominate_radius.begin());
}

unknown_x_baseline::unknown_x_baseline(const int dim, const int K, vector<float>& userPref, Rtree& a_rtree, float** pg){
    dimen=dim;
    ones=vector<float>(dimen, 1);
    zeros=vector<float>(dimen, 0);
    zeros_node=new unknown_X_node(dimen, zeros.data(), -1);
    rt=new unknown_X_node(dimen, ones.data(), a_rtree.m_memory.m_rootPageID);
    heap.emplace(INFINITY, rt);
    to_dl.push_back(zeros_node);
    to_dl.push_back(rt);
    k=K;
    userpref=userPref;
    PG=pg;
    next=0;
}

pair<int, float> unknown_x_baseline::get_next(){
    next++;
    if(interval.size()>=next){
        return interval[next-1];
    }
    while (!heap.empty())
    {
        if(interval.size()>=next){
            return interval[next-1];
        }
        unknown_X_node *popped_node=heap.begin()->second;
        popped_node->fetched=true;
        int pageID = popped_node->page_id;
        heap.erase(heap.begin());
        if (pageID > MAXPAGEID)  // option processing
        {
            if (interval.size() < k)  // Phase (1)
            {
                interval.emplace_back(pageID - MAXPAGEID, 0);
                if(interval.size()==k) // Phase (2)
                {
                    //init S
                    for (pair<int, float> &option:interval) {
                        incompSet.push_back(option.first);
                    }
                    for(pair<const float, unknown_X_node *> &ele:heap)
                    {
                        unknown_X_node *node=ele.second;
                        node->init_radius(incompSet, PG, userpref, k);
                        S.insert(node);
                    }
                }
                return interval.back();
            }
            else
            {
                //update all element in S
                assert(!S.empty());
                for (unknown_X_node* node:S)
                {
                    if (!node->fetched)
                    {
                        node->update_radius_erase(popped_node->data, userpref);
                    }
                }
                incompSet.push_back(pageID - MAXPAGEID);
            }
        }
        else // internal and leaf nodes processing
        {
            S.erase(popped_node);
            RtreeNode* node = ramTree[pageID];
            if (node->isLeaf())
            {
                for (int i = 0; i < node->m_usedspace; i++)
                {
                    float tmpScore = 0;
                    for (int j = 0; j < dimen; j++)
                    {
                        pt[j] = node->m_entry[i]->m_hc.getCenter()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    unknown_X_node *tmp_node=new unknown_X_node(dimen, PG[node->m_entry[i]->m_id], node->m_entry[i]->m_id + MAXPAGEID);
                    to_dl.push_back(tmp_node);
                    heap.emplace(tmpScore, tmp_node);
                    if(interval.size()>=k) {
                        tmp_node->init_radius(incompSet, PG, userpref, k);
                        S.insert(tmp_node);
                    }
                }
            }
            else
            {
                for (int i = 0; i < node->m_usedspace; i++)
                {
                    float tmpScore = 0;
                    for (int j = 0; j < dimen; j++)
                    {
                        pt[j] = node->m_entry[i]->m_hc.getUpper()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    const float *ptr=node->m_entry[i]->m_hc.getUpper().m_coor;
                    unknown_X_node *tmp_node=new unknown_X_node(dimen, ptr, node->m_entry[i]->m_id);
                    to_dl.push_back(tmp_node);
                    heap.emplace(tmpScore, tmp_node);
                    if(interval.size()>=k) {
                        tmp_node->init_radius(incompSet, PG, userpref, k);
                        S.insert(tmp_node);
                    }
                }
            }

        }
        if(interval.size()>=k) {
            pair<float, unknown_X_node *> LB(INFINITY, zeros_node);
            for (unknown_X_node *node:S) {
                if (node->radius_LB() < LB.first) {
                    LB.first = node->radius_LB();
                    LB.second = node;
                }
            }
            while (LB.second->page_id > MAXPAGEID && LB.second->fetched) {
                // if LB is an option and is updated, add it to result list "interval"
                interval.emplace_back(LB.second->page_id - MAXPAGEID, LB.second->radius_LB());
                S.erase(LB.second);
                LB.first = INFINITY;
                LB.second = zeros_node;
                for (unknown_X_node *node:S) {
                    if (node->radius_LB() < LB.first) {
                        LB.first = node->radius_LB();
                        LB.second = node;
                    }
                }
            }
        }
    }
}

unknown_x_baseline::~unknown_x_baseline(){
    for(unknown_X_node *node:to_dl){
        delete (node);
    }
}

float computeRho_unknownX_basic(const int dimen, const int k, const int X, vector<float>& userpref, Rtree& a_rtree, float** PG)
{
    // phase (1) if get_next_time<=k, from BBS fetch topK
    // phase (2) for each element in BBS, calculate their inflection radius and store them into unordered_set S
    // phase (3) while the the top of S is not an option:
    //          (a) pop and push node A out and into BBS heap
    //          (b) if node A is an option:
    //                  not update S
    //                  marked A as fetched
    //              else
    //                  update S to remove node
    //          (c) update all nodes in S such that not fetched by BBS yet (use a flag FETCH to record), update LB
    vector<pair<int, float>> interval; // return
    multimap<float, unknown_X_node*, greater<float>> heap; //BBS heap, <w\cdot node, node>, if node is an option then node=id+MAXPAGEID
    unordered_set<unknown_X_node*> S;
    vector<long int> incompSet;
    float pt[MAXDIMEN];
    vector<float> ones(dimen, 1);
    vector<float> zeros(dimen, 0);
    unknown_X_node *zeros_node=new unknown_X_node(dimen, zeros.data(), -1);
    unknown_X_node *rt=new unknown_X_node(dimen, ones.data(), a_rtree.m_memory.m_rootPageID);
    heap.emplace(INFINITY, rt);
    vector<unknown_X_node*> to_dl; // todo change into intel pointer
    to_dl.push_back(zeros_node);
    to_dl.push_back(rt);
    while (!heap.empty() && interval.size()<X)
    {
        unknown_X_node *popped_node=heap.begin()->second;
        popped_node->fetched=true;
        int pageID = popped_node->page_id;
        heap.erase(heap.begin());
        if (pageID > MAXPAGEID)  // option processing
        {
            if (interval.size() < k)  // Phase (1)
            {
                interval.emplace_back(pageID - MAXPAGEID, 0);
                if(interval.size()==k) // Phase (2)
                {
                    //init S
                    for (pair<int, float> &option:interval) {
                        incompSet.push_back(option.first);
                    }
                    for(pair<const float, unknown_X_node *> &ele:heap)
                    {
                        unknown_X_node *node=ele.second;
                        node->init_radius(incompSet, PG, userpref, k);
                        S.insert(node);
                    }
                }
            }
            else
            {
                if (interval.size() < X )  // should get_next X times
                {
                    //update all element in S
                    assert(!S.empty());
                    for (unknown_X_node* node:S)
                    {
                        if (!node->fetched)
                        {
                            node->update_radius_erase(popped_node->data, userpref);
                        }
                    }
                }
                else if(X<=k)
                {
                    assert(X==k);// if fails there is a problem in data
                    break;
                }
                else   // interval.size() == X, should begin to return
                {
                    break;
                }
                incompSet.push_back(pageID - MAXPAGEID);
            }
        }
        else // internal and leaf nodes processing
        {
            S.erase(popped_node);
//            delete(popped_node);
            RtreeNode* node = ramTree[pageID];
            if (node->isLeaf())
            {
                for (int i = 0; i < node->m_usedspace; i++)
                {
                    float tmpScore = 0;
                    for (int j = 0; j < dimen; j++)
                    {
                        pt[j] = node->m_entry[i]->m_hc.getCenter()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    unknown_X_node *tmp_node=new unknown_X_node(dimen, PG[node->m_entry[i]->m_id], node->m_entry[i]->m_id + MAXPAGEID);
                    to_dl.push_back(tmp_node);
                    heap.emplace(tmpScore, tmp_node);
                    if(interval.size()>=k) {
                        tmp_node->init_radius(incompSet, PG, userpref, k);
                        S.insert(tmp_node);
                    }
                }
            }
            else
            {
                for (int i = 0; i < node->m_usedspace; i++)
                {
                    float tmpScore = 0;
                    for (int j = 0; j < dimen; j++)
                    {
                        pt[j] = node->m_entry[i]->m_hc.getUpper()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    const float *ptr=node->m_entry[i]->m_hc.getUpper().m_coor;
                    unknown_X_node *tmp_node=new unknown_X_node(dimen, ptr, node->m_entry[i]->m_id);
                    to_dl.push_back(tmp_node);
                    heap.emplace(tmpScore, tmp_node);
                    if(interval.size()>=k) {
                        tmp_node->init_radius(incompSet, PG, userpref, k);
                        S.insert(tmp_node);
                    }
                }
            }

        }
        if(interval.size()>=k) {
            pair<float, unknown_X_node *> LB(INFINITY, zeros_node);
            for (unknown_X_node *node:S) {
                if (node->radius_LB() < LB.first) {
                    LB.first = node->radius_LB();
                    LB.second = node;
                }
            }
            while (LB.second->page_id > MAXPAGEID && LB.second->fetched) {
                // if LB is an option and is updated, add it to result list "interval"
                interval.emplace_back(LB.second->page_id - MAXPAGEID, LB.second->radius_LB());
                S.erase(LB.second);
//                delete(LB.second);
                if (interval.size() == X) {
                    break;
                }
                LB.first = INFINITY;
                LB.second = zeros_node;
                for (unknown_X_node *node:S) {
                    if (node->radius_LB() < LB.first) {
                        LB.first = node->radius_LB();
                        LB.second = node;
                    }
                }
            }
        }
    }

    for(unknown_X_node *node:to_dl){
        delete (node);
    }
//    delete(zeros_node);
    if(X<=k){
        return 0;
    }else if(interval.size()<X){
        return INFINITY;
    }else{
        return interval.back().second;
    }
}

float computeRho_unknownX_efficient(const int dimen, const int k, const int X, vector<float>& userpref, Rtree& a_rtree, float* PG[])
{
    // phase (1) if get_next_time<=k, from BBS fetch topK
    // phase (2) for each element in BBS, calculate their inflection radius and store them into set S,  min_heap Q


    typedef float RADIUS;  // is the inflection radius
    typedef int PAGE_ID;  // page id in rtree
    typedef int PG_IDX;  // idx in PG
    typedef long PG_IDX_L;  // idx in PG
    typedef float DOT;   // is the result of dot product with w
    typedef unknown_X_node* NODE_PTR;

    vector<pair<PG_IDX, RADIUS>> interval; // return
    multimap<DOT, NODE_PTR, greater<DOT>> heap; //BBS max_heap, <w\cdot node, node>, if node is an option then node=id+MAXPAGEID
    unordered_set<NODE_PTR> S; // reflect which nodes and options in BBS
    multimap<RADIUS, NODE_PTR, less<RADIUS>> Q; // min_heap based on inflection radius, lazy update for S
    multimap<RADIUS, NODE_PTR, less<RADIUS>> C; // min_heap based on inflection radius, candidate list
    vector<PG_IDX_L> incompSet;
    float pt[MAXDIMEN];
    float radius = INFINITY;
    vector<float> ones(dimen, 1);
    NODE_PTR rt=new unknown_X_node(dimen, ones.data(), a_rtree.m_memory.m_rootPageID);
    heap.emplace(INFINITY, rt);

    while (!heap.empty() && interval.size()<X)
    {
        NODE_PTR popped_node=heap.begin()->second;
        PAGE_ID pageID = popped_node->page_id;
        heap.erase(heap.begin());
        if(interval.size()>=k){
            S.erase(popped_node);
        }

        if (pageID > MAXPAGEID)  // option processing
        {
            if (interval.size() < k)  // Phase (1)
            {
                interval.emplace_back(pageID - MAXPAGEID, 0);
                if(interval.size()==k) // Phase (2)
                {
                    //init S, init Q
                    for (pair<PG_IDX, RADIUS> &option:interval) {
                        incompSet.push_back(option.first);
                    }
                    for(pair<const DOT, NODE_PTR> &ele:heap)
                    {
                        NODE_PTR node=ele.second;
                        node->init_radius(incompSet, PG, userpref, k);
                        S.insert(node);
                        Q.emplace(node->radius_LB(), node);
                    }
                }
            }
            else
            {
                if (interval.size() < X )  // should get_next X times
                {
                    popped_node->update_radius(incompSet.begin()+popped_node->tau, incompSet.end(), PG, userpref);
                    C.emplace(popped_node->radius_LB(), popped_node);
                    incompSet.push_back(pageID - MAXPAGEID);
                }
                else if(X<=k)
                {
                    assert(X==k);// if fails there is a problem in data
                    radius=0;
                    break;
                }
                else   // interval.size() == X, should begin to return
                {
                    radius=interval.back().second;
                    break;
                }
            }
        }
        else // internal and leaf nodes processing
        {
            float possible_next_radius=INFINITY;
            if(!C.empty()){
                possible_next_radius=C.begin()->first;
            }
            RtreeNode* node = ramTree[pageID];
            if (node->isLeaf())
            {
                for (int i = 0; i < node->m_usedspace; i++)
                {
                    DOT tmpScore = 0;
                    for (int j = 0; j < dimen; j++)
                    {
                        pt[j] = node->m_entry[i]->m_hc.getCenter()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    unknown_X_node *tmp_node=new unknown_X_node(dimen, PG[node->m_entry[i]->m_id], node->m_entry[i]->m_id + MAXPAGEID);
                    heap.emplace(tmpScore, tmp_node);
                    if(interval.size()>=k) {
                        S.insert(tmp_node);
                        tmp_node->init_radius(incompSet, PG, userpref, k, possible_next_radius);
                        Q.emplace(tmp_node->radius_LB(), tmp_node);
                    }
                }
            }
            else
            {
                for (int i = 0; i < node->m_usedspace; i++)
                {
                    DOT tmpScore = 0;
                    for (int j = 0; j < dimen; j++)
                    {
                        pt[j] = node->m_entry[i]->m_hc.getUpper()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    const float *ptr=node->m_entry[i]->m_hc.getUpper().m_coor;
                    unknown_X_node *tmp_node=new unknown_X_node(dimen, ptr, node->m_entry[i]->m_id);
                    heap.emplace(tmpScore, tmp_node);
                    if(interval.size()>=k) {
                        S.insert(tmp_node);
                        tmp_node->init_radius(incompSet, PG, userpref, k, possible_next_radius);
                        Q.emplace(tmp_node->radius_LB(), tmp_node);
                    }
                }
            }

        }
        if(interval.size()>=k && !C.empty()) {
            _Rb_tree_iterator<pair<const RADIUS, NODE_PTR>> possible_next = C.begin();
            // a lazy update with S
            while (!Q.empty() && S.find(Q.begin()->second) == S.end()) {
                if (Q.begin()->second->page_id <= MAXPAGEID) { // directly delete an inter node
                    delete (Q.begin()->second);
                }
                Q.erase(Q.begin());
            }

            // make sure the top of Q is updated or its inflection radius is larger than possible_next.inflection radius
            while (!Q.empty() &&
                   !(Q.begin()->second->update(incompSet.size()) || Q.begin()->first >= possible_next->first)) {
                NODE_PTR qnode = Q.begin()->second;

                // a lazy update with tau
                qnode->update_radius(incompSet.begin() + qnode->tau, incompSet.end(), PG, userpref,
                                     possible_next->first);
                Q.erase(Q.begin());
                Q.emplace(qnode->radius_LB(), qnode);
                while (!Q.empty() && S.find(Q.begin()->second) == S.end()) { // a lazy update with S
                    if (Q.begin()->second->page_id <= MAXPAGEID) { // directly delete an inter node
                        delete (Q.begin()->second);
                    }
                    Q.erase(Q.begin());
                }
            }


            // if Q empty, add element in C into interval one by one and return this function
            if (Q.empty()) {
                // begin getting ready to return
                while (interval.size() < X && !C.empty()) {
                    interval.emplace_back(C.begin()->second->page_id - MAXPAGEID, C.begin()->first);
                    delete (C.begin()->second);
                    C.erase(C.begin());
                }
                if (interval.size() >= X) {
                    radius = interval.back().second;
                }
                break;
            }
            // if Q not empty, then check whether possible_next.if_r is lower than Q.top.if_r
            // if possible_next.if_r is lower than Q.top.if_r:
            //     add possible_next into result list "interval"
            // else:
            //     continue fetch nodes or options with BBS
            if (possible_next->first <= Q.begin()->first) {
                interval.emplace_back(C.begin()->second->page_id - MAXPAGEID, C.begin()->first);
                delete (C.begin()->second);
                C.erase(C.begin());
                // if still can fetch from candidate list C
                while (!C.empty() && interval.size() < X) {
                    possible_next = C.begin();
                    if (possible_next->first <= Q.begin()->first) {
                        interval.emplace_back(C.begin()->second->page_id - MAXPAGEID, C.begin()->first);
                        delete (C.begin()->second);
                        C.erase(C.begin());
                    } else {
                        break;
                    }
                }
            }
        }
    }

    for(NODE_PTR node:S){
        delete (node);
    }
    for(pair<const RADIUS , NODE_PTR> node_iter:C){
        delete (node_iter.second);
    }
    if(interval.size()==X && X>=k){
        radius=interval.back().second;
    }
    return radius;
}

unknown_x_efficient::unknown_x_efficient(const int dim, int K, vector<float> &userPref, Rtree &aRtree, float **pg
) : a_rtree(aRtree) {
    this->dimen=dim;
    this->k=K;
    this->userpref=userPref;
    this->PG=pg;
    vector<float> ones(dimen, 1);
    NODE_PTR rt=new unknown_X_node(dimen, ones.data(), a_rtree.m_memory.m_rootPageID);
    heap.emplace(INFINITY, rt);
}

unknown_x_efficient::~unknown_x_efficient(){
    for(NODE_PTR node:S){
        delete (node);
    }
    for(pair<const RADIUS , NODE_PTR> node_iter:C){
        delete (node_iter.second);
    }
}

inline bool unknown_X_node::update() const{
    return this->tau>=this->needed_to_update;
}

pair<int, float> unknown_x_efficient::get_next() {
    while (!heap.empty() || !C.empty()) {
        if (interval.size() >= k && !C.empty()) {
            _Rb_tree_iterator<pair<const RADIUS, NODE_PTR>> possible_next = C.begin();

            // a lazy update with S
            while (!Q.empty() && S.find(Q.begin()->second) == S.end()) {
                if (Q.begin()->second->page_id <= MAXPAGEID) { // directly delete an inter node
                        delete (Q.begin()->second);
                }
                Q.erase(Q.begin());
            }

            // make sure the top of Q is updated or its inflection radius is larger than possible_next.inflection radius
            while (!Q.empty() &&
                   !(Q.begin()->second->update(incompSet.size()) || Q.begin()->first >= possible_next->first)) {
                NODE_PTR qnode = Q.begin()->second;

                // a lazy update with tau
                qnode->update_radius(incompSet.begin() + qnode->tau, incompSet.end(), PG, userpref,
                                     possible_next->first);

                Q.erase(Q.begin());
                Q.emplace(qnode->radius_LB(), qnode);
                while (!Q.empty() && S.find(Q.begin()->second) == S.end()) { // a lazy update with S
                    if (Q.begin()->second->page_id <= MAXPAGEID) { // directly delete an inter node
                        delete (Q.begin()->second);
                    }
                    Q.erase(Q.begin());
                }
            }

            // if Q empty, add element in C into interval one by one and return this function
            // if Q not empty, then check whether possible_next.if_r is lower than Q.top.if_r
            // if possible_next.if_r is lower than Q.top.if_r:
            //     add possible_next into result list "interval"
            // else:
            //     continue fetch nodes or options with BBS
            if (Q.empty() || possible_next->first <= Q.begin()->first) {
                // begin getting ready to return
                interval.emplace_back(C.begin()->second->page_id - MAXPAGEID, C.begin()->first);
//                assert(C.begin()->second->page_id>MAXPAGEID);
                delete (C.begin()->second);
                C.erase(C.begin());
                while (C.size() > 1 && !C.begin()->second->update()) {
                    unknown_X_node *ni = C.begin()->second;
                    float lb = (++C.begin())->first;
                    ni->update_radius(incompSet.begin() + ni->tau, incompSet.begin() + ni->needed_to_update, PG, userpref, lb);
                    C.erase(C.begin());
                    C.emplace(ni->radius_LB(), ni);
                }
                if (!C.empty() &&!C.begin()->second->update()) {
                    unknown_X_node *ni = C.begin()->second;
                    ni->update_radius(incompSet.begin() + ni->tau, incompSet.begin() + ni->needed_to_update, PG, userpref);
                    C.erase(C.begin());
                    C.emplace(ni->radius_LB(), ni);
                }
                return interval.back();
            }
        }
        NODE_PTR popped_node = heap.begin()->second;
        PAGE_ID pageID = popped_node->page_id;
        heap.erase(heap.begin());
        if (interval.size() >= k) {
            S.erase(popped_node);
        }
        if (pageID > MAXPAGEID)  // option processing
        {
            if (interval.size() < k)  // Phase (1)
            {
                delete (popped_node);
                interval.emplace_back(pageID - MAXPAGEID, 0);
                if (interval.size() == k) // Phase (2)
                {
                    //init S, init Q
                    for (pair<PG_IDX, RADIUS> &option:interval) {
                        incompSet.push_back(option.first);
                    }
                    for (pair<const DOT, NODE_PTR> &ele:heap) {
                        NODE_PTR node = ele.second;
                        node->init_radius(incompSet, PG, userpref, k);
                        S.insert(node);
                        Q.emplace(node->radius_LB(), node);
                    }
                }
                return interval.back();
            } else {
                // UPDATE WHEN needed
                if(!C.empty()) {
                    popped_node->update_radius(incompSet.begin() + popped_node->tau, incompSet.end(), PG, userpref,
                                               C.begin()->first);
                }else{
                    popped_node->update_radius(incompSet.begin() + popped_node->tau, incompSet.end(), PG, userpref);
                }
                popped_node->needed_to_update=incompSet.size();
                C.emplace(popped_node->radius_LB(), popped_node);
                while (C.size() > 1 && !C.begin()->second->update()) {
                    unknown_X_node *ni = C.begin()->second;
                    float lb = (++C.begin())->first;
                    ni->update_radius(incompSet.begin() + ni->tau, incompSet.begin() + ni->needed_to_update, PG, userpref, lb);
                    C.erase(C.begin());
                    C.emplace(ni->radius_LB(), ni);
                }
                if (!C.empty() &&!C.begin()->second->update()) {
                    unknown_X_node *ni = C.begin()->second;
                    ni->update_radius(incompSet.begin() + ni->tau, incompSet.begin() + ni->needed_to_update, PG, userpref);
                    C.erase(C.begin());
                    C.emplace(ni->radius_LB(), ni);
                }
                if(popped_node->radius_LB()!=INFINITY)
                incompSet.push_back(pageID - MAXPAGEID);
            }
        } else // internal and leaf nodes processing
        {
            float possible_next_radius = INFINITY;
            if (!C.empty()) {
                possible_next_radius = C.begin()->first;
            }
            RtreeNode *node = ramTree[pageID];
            if (node->isLeaf()) {
                for (int i = 0; i < node->m_usedspace; i++) {
                    DOT tmpScore = 0;
                    for (int j = 0; j < dimen; j++) {
                        pt[j] = node->m_entry[i]->m_hc.getCenter()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    unknown_X_node *tmp_node = new unknown_X_node(dimen, PG[node->m_entry[i]->m_id],
                                                                  node->m_entry[i]->m_id + MAXPAGEID);
                    heap.emplace(tmpScore, tmp_node);
                    if (interval.size() >= k) {
                        S.insert(tmp_node);
                        tmp_node->init_radius(incompSet, PG, userpref, k, possible_next_radius);
                        Q.emplace(tmp_node->radius_LB(), tmp_node);
                    }
                }
            } else {
                for (int i = 0; i < node->m_usedspace; i++) {
                    DOT tmpScore = 0;
                    for (int j = 0; j < dimen; j++) {
                        pt[j] = node->m_entry[i]->m_hc.getUpper()[j];
                        tmpScore += pt[j] * userpref[j];
                    }
                    const float *ptr = node->m_entry[i]->m_hc.getUpper().m_coor;
                    unknown_X_node *tmp_node = new unknown_X_node(dimen, ptr, node->m_entry[i]->m_id);
                    heap.emplace(tmpScore, tmp_node);
                    if (interval.size() >= k) {
                        S.insert(tmp_node);
                        tmp_node->init_radius(incompSet, PG, userpref, k, possible_next_radius);
                        Q.emplace(tmp_node->radius_LB(), tmp_node);
                    }
                }
            }

        }

    }
    return {-1, INFINITY};
}


vector<int> build_qhull(const vector<int> &opt_idxes, float **PG, vector<vector<double>> &square_vertexes){
    // \tpara ITERATABLE set<int>, unorder_set<int>, vector<int> and other iteratable STL<INT> CLASS
    int dim=square_vertexes[0].size();
//    for (int opt_idx:opt_idxes) {
//        for (int i = 0; i < dim; ++i) {
//            square_vertexes[i][i] = max(square_vertexes[i][i],PG[opt_idx][i]+SIDELEN);
//        }
//    }
//    while(square_vertexes.size()>dim+1){
//        square_vertexes.pop_back();
//    }
    square_vertexes.clear();
    square_vertexes.emplace_back(dim);
    for (int opt_idx:opt_idxes) {
        for (int i = 0; i < dim; ++i) {
                vector<double> tmp(PG[opt_idx], PG[opt_idx]+dim);
                for(auto &t:tmp){
                    t+=SIDELEN;
                    t=max(t, 0);// deal with precision problem such as -1e-14, change it into 0
                }
                tmp[i]=0;
                square_vertexes.push_back(tmp);
        }
    }
    string s = to_string(dim) + " " + to_string(opt_idxes.size() + square_vertexes.size()) + " ";
    for(int opt_idx:opt_idxes){
        for (int i = 0; i <dim ; ++i) {
//            assert(opt_idx>=0 && opt_idx<=objCnt);
            if(PG[opt_idx][i]+SIDELEN < 1e-6){
                s += to_string(SIDELEN)+ " ";// in case of precision problem
            }else{
                s += to_string(PG[opt_idx][i]+SIDELEN) + " ";
            }
        }
    }
    for (vector<double> & square_vertex : square_vertexes){
        for (float j : square_vertex){
            s += to_string(j) + " ";
        }
    }
    istringstream is(s);
    RboxPoints rbox;
    rbox.appendPoints(is);
    Qhull q(rbox, "QJ");
    qhull_user qu;
    return qu.get_CH_pointID(q, opt_idxes);
}

vector<int> build_qhull(const vector<int> &opt_idxes, vector<vector<float>> &PG, vector<vector<double>> &square_vertexes){
    // \tpara ITERATABLE set<int>, unorder_set<int>, vector<int> and other iteratable STL<INT> CLASS
    int dim=square_vertexes[0].size();
//    for (int opt_idx:opt_idxes) {
//        for (int i = 0; i < dim; ++i) {
//            square_vertexes[i][i] = max(square_vertexes[i][i],PG[opt_idx][i]+SIDELEN);
//        }
//    }
//    while(square_vertexes.size()>dim+1){
//        square_vertexes.pop_back();
//    }
    square_vertexes.clear();
    square_vertexes.emplace_back(dim);
    for (int opt_idx:opt_idxes) {
        for (int i = 0; i < dim; ++i) {
                vector<double> tmp(PG[opt_idx].begin(), PG[opt_idx].end());
                for(auto &t:tmp){
                    t+=SIDELEN;
                    t=max(t, 0);
                }
                tmp[i]=0;
                square_vertexes.push_back(tmp);
        }
    }
    string s = to_string(dim) + " " + to_string(opt_idxes.size() + square_vertexes.size()) + " ";
    for(int opt_idx:opt_idxes){
//        assert(opt_idx<=objCnt);
        for (int i = 0; i <dim ; ++i) {
            if(PG[opt_idx][i]+SIDELEN < 1e-6){
                s += to_string(SIDELEN)+ " ";// in case of precision problem
            }else{
                s += to_string(PG[opt_idx][i]+SIDELEN) + " ";
            }
        }
    }
    for (vector<double> & square_vertex : square_vertexes){
//        assert(square_vertex.size()==dim);
        for (float j : square_vertex){
            s += to_string(j) + " ";
        }
    }
    istringstream is(s);
    RboxPoints rbox;
    rbox.appendPoints(is);
    Qhull q(rbox, "QJ");
    qhull_user qu;
    return qu.get_CH_pointID(q, opt_idxes);
}

void test_build_qhull(){
    vector<vector<float>> pg(6, vector<float>(3));
    pg[4][0]=1.0;
    pg[1][1]=1.0;
    pg[2][0]=-1.0;
    pg[3][1]=-1.0;
    pg[0][2]=1.0;
    pg[5][2]=-1.0;
    vector<int> i(6);
    for (int j = 0; j < 6; ++j) {
        i[j]=j;
    }
    vector<vector<double>> sq(3, vector<double>(3));
    auto CH=build_qhull(i, pg, sq);
    for(auto j:CH){
        cout<<j<<endl;
    }
}

void top_region(const vector<int> &opt_idxes, float **PG, vector<vector<double>> &square_vertexes,
                                                            unordered_map<int, vector<vector<double>>> &ret){
    int dim=square_vertexes[0].size();
//    for (int opt_idx:opt_idxes) {
//        for (int i = 0; i < dim; ++i) {
//            square_vertexes[i][i] = max(square_vertexes[i][i], PG[opt_idx][i]+SIDELEN);
//        }
//    }
//    while(square_vertexes.size()>dim+1){
//        square_vertexes.pop_back();
//    }
    square_vertexes.clear();
    square_vertexes.emplace_back(dim);
    for (int opt_idx:opt_idxes) {
        for (int i = 0; i < dim; ++i) {
                vector<double> tmp(PG[opt_idx], PG[opt_idx]+dim);
                for(auto &t:tmp){
                    t+=SIDELEN;
                    t=max(t, 0);
                }
                tmp[i]=0;
                square_vertexes.push_back(tmp);
        }
    }
    string s = to_string(dim) + " " + to_string(opt_idxes.size() + square_vertexes.size()) + " ";
    for(int opt_idx:opt_idxes){
        for (int i = 0; i <dim ; ++i) {
//            assert(opt_idx>=0 && opt_idx<=objCnt);
            if(PG[opt_idx][i]+SIDELEN < 1e-6){
                s += to_string(SIDELEN)+ " ";
            }else{
                s += to_string(PG[opt_idx][i]+SIDELEN) + " "; // in case of precision problem
            }
        }
    }
    for (vector<double> & square_vertex : square_vertexes){
        for (float j : square_vertex){
            s += to_string(j) + " ";
        }
    }
    istringstream is(s);
    RboxPoints rbox;
    rbox.appendPoints(is);
    Qhull q(rbox, "QJ");
    qhull_user qu;
    unordered_map<int, vector<vector<double>>> points;
    qu.get_neiFacetsNorm_of_point(q, opt_idxes, points);
    for (auto &pt:points) {
        ret[pt.first]=points_to_halfspace(pt.second);
    }
}

unordered_map<int, vector<vector<double>>> top_region(const vector<int> &opt_idxes, float **PG, vector<vector<double>> &square_vertexes){
    //
    unordered_map<int, vector<vector<double>>> ret;
    top_region(opt_idxes, PG, square_vertexes, ret);
    return ret;
}


void build_qhull(const vector<int> &opt_idxes, float **PG, vector<vector<double>> &square_vertexes, Qhull *q_ptr){
//    assert(!square_vertexes.empty());
    int dim=square_vertexes[0].size();
//    for (int opt_idx:opt_idxes) {
//        for (int i = 0; i < dim; ++i) {
//            square_vertexes[i][i] = max(square_vertexes[i][i], PG[opt_idx][i]+SIDELEN);
//        }
//    }
//    while(square_vertexes.size()>dim+1){
//        square_vertexes.pop_back();
//    }
    square_vertexes.clear();
    square_vertexes.emplace_back(dim);
    for (int opt_idx:opt_idxes) {
        for (int i = 0; i < dim; ++i) {
                vector<double> tmp(PG[opt_idx], PG[opt_idx]+dim);
                for(auto &t:tmp){
                    t+=SIDELEN;
                    t=max(t, 0);
                }
                tmp[i]=0;
                square_vertexes.push_back(tmp);
        }
    }
    string s = to_string(dim) + " " + to_string(opt_idxes.size() + square_vertexes.size()) + " ";
    for(int opt_idx:opt_idxes){
        for (int i = 0; i <dim ; ++i) {
            if(PG[opt_idx][i]+SIDELEN < 1e-6){
                s += to_string(SIDELEN)+ " ";// in case of precision problem
            }else{
                s += to_string(PG[opt_idx][i]+SIDELEN) + " ";
            }
        }
    }
    for (vector<double> & square_vertex : square_vertexes){
        for (float j : square_vertex){
            s += to_string(j) + " ";
        }
    }
    istringstream is(s);
    RboxPoints rbox;
    rbox.appendPoints(is);
    q_ptr->runQhull(rbox, "QJ");
}

inline void update_square_vertexes(vector<vector<double>> &square_vertexes, float *new_ele, int dim){
    for (int i=0;i<dim;++i){
        square_vertexes[i][i]=max(square_vertexes[i][i], new_ele[i]);
    }
}

class ch{
    unordered_set<int> rest;
    unordered_map<int, int> pdtid_layer;
    unordered_map<int, vector<int>> A_p;
    vector<vector<int>> chs;
    vector<int> EMPTY;
    float** pointSet;
    int d;
    public:
    vector<int> rskyband;
    ch(vector<int> &idxes, float** &point_set, int dim){
        this->rskyband=idxes;
        this->rest=unordered_set<int>(idxes.begin(), idxes.end());
        this->pointSet=point_set;
        this->d=dim;
    }
    const vector<int>& get_next_layer(){
        vector<vector<double>> square_vertexes(d+1, vector<double>(d));
        vector<int> rest_v(rest.begin(), rest.end());
        Qhull q;
        build_qhull(rest_v, pointSet, square_vertexes, &q);
        qhull_user qu;
        vector<int> ch=qu.get_CH_pointID(q, rest_v);

//        Qhull q_test;
//        build_qhull(ch, pointSet, square_vertexes, &q_test);
//        vector<int> ch_test=qu.get_CH_pointID(q_test, ch);
//        assert(ch_test.size()==ch.size());

        chs.push_back(ch);
        for (int idx:ch) {
//            assert(idx>=0 && idx<=objCnt);
            pdtid_layer[idx] =  chs.size();
            rest.erase(idx);
        }
        qu.get_neiVT_of_VT(q, rest_v, A_p);
//        for (int idx:ch) {
////            assert(A_p.find(idx)!=A_p.end());
//        }
        return chs.back();
    }

    int get_option_layer(int option){
//        assert(option>=0 && option <=objCnt);
        auto iter=pdtid_layer.find(option);
        if(iter!=pdtid_layer.end()){
            return iter->second;
        }else{
            return -1; // not in current i layers
        }
    }

    const vector<int>& get_neighbor_vertex(int option){
//        assert(option>=0 && option <=objCnt);
        auto lazy_get=A_p.find(option);
        if(lazy_get!=A_p.end()){
            return lazy_get->second;
        }else{
            return EMPTY;
        }
    }

    const vector<int>& get_layer(int which_layer){
        // layer is starting from 1
        while(chs.size()<which_layer && !rest.empty()){
            this->get_next_layer();
        }
        if(chs.size()<which_layer){
            return EMPTY;
        }
        return this->chs[which_layer-1];
    }

    ~ch(){
    }
};

bool region_overlap(vector<vector<double>> &r1, vector<vector<double>> &r2, vector<vector<double>> &r1_r2){
    return isFeasible(r1, r2, r1_r2);
}

float dist_region_w(vector<vector<double>> &region, vector<float> &w){
//    qp_solver q(w, region);
//    return q.qp_solve();
//    auto begin = chrono::steady_clock::now();
//  finally, we choose quadProd_lib
    float ret=qp_solver2(w, region);
//    auto now = chrono::steady_clock::now();
//    chrono::duration<double> elapsed_seconds= now-begin;
//    cout << "Total time cost: " << elapsed_seconds.count()<< " SEC " << endl;
    return ret;
}

double dist_region_w(vector<vector<double>> &r1,vector<vector<double>> &r2,  vector<float> &w){
    double ret=qp_solver2(w, r1, r2);
    return ret;
}

void topRegions(vector<vector<double>> &parent_region, const vector<int> &CH_upd, ch &ch_obj,
                multimap<double, region*> &id_radius, int deepest_layer, float **PG, int dim,
                const int k, vector<int> &topi, vector<float> &w, vector<int> &neighbors){
    //dfs
    if(topi.size()==k){
        double dist=dist_region_w(parent_region, w);
        if(dist!=INFINITY){
            region *r=new region(topi, parent_region);
            id_radius.emplace(dist, r);
        }
        if(id_radius.size()%1000==0){
            if(true){
                cout<< id_radius.size()<<"\n";
                cout<<"top:";
                for (int i:topi) {
                    cout<<i<<" ";
                }
                cout<<"\n";
                cout<< deepest_layer << " " << CH_upd.size()<< " " << neighbors.size() << "\n";
            }
        }
        return;
    }
    int d=dim;
    vector<vector<double>> square_vertexes(d+1, vector<double>(d));
    unordered_map<int, vector<vector<double>>> tops_region=top_region(CH_upd, PG, square_vertexes);
    for(auto &opt_r: tops_region){
        vector<vector<double>> new_region;
        if(region_overlap(parent_region, opt_r.second, new_region)){
            // update topi
            vector<int> topi1=topi;
//            assert(opt_r.first>=0 && opt_r.first<=objCnt);
            topi1.push_back(opt_r.first);
            // append neighbor, should be a set in nature
            vector<int> new_nei=neighbors;
            const vector<int> &aps=ch_obj.get_neighbor_vertex(opt_r.first); // TOREAD
            for(int ap:aps){
                new_nei.push_back(ap);
            }
            // judge whether need next CH layers
            int deeper=max(deepest_layer, ch_obj.get_option_layer(opt_r.first));
            unordered_set<int> update_s(new_nei.begin(), new_nei.end());
            const vector<int> &CH_mp1=ch_obj.get_layer(deeper+1);
            for(int next_layer_opt:CH_mp1){
                update_s.insert(next_layer_opt);
            }
            for (int i:topi1) {
                update_s.erase(i);
            }
            vector<int> update(update_s.begin(), update_s.end());
            topRegions(new_region, update, ch_obj,id_radius, deeper, PG, dim,k, topi1, w, new_nei);
        }
    }

}


int topRegions_efficient(vector<vector<double>> &parent_region, ch &ch_obj,
                multimap<double, region*> &id_radius, float **PG, int dim, int X,
                const int k, vector<float> &w, unordered_set<int> &top1_calculated,
                vector<pair<int, double>> &utk_option_ret,
                vector<pair<double, region*>> &utk_cones_ret,
                unordered_map<int, vector<vector<double>>> &top1_region){
    // init "top1_calculated" with top1 respecting to w
    auto begin=chrono::steady_clock::now();
    unordered_set<int> options;
    for (int i:top1_calculated) {
        options.insert(i);
        utk_option_ret.emplace_back(i, 0);
        cout << "radius: " << 0 << "\n";
    }
    int cnt=0;
    cout<<parent_region.size()<<endl;
    while(options.size() < X && !id_radius.empty()){
        pair<double, region*> popped=*id_radius.begin(); // must deep copy because next line we use erase
        id_radius.erase(id_radius.begin());
//        for(int topi: popped.second->topk){
//            auto iter=options.find(topi);
//            if(iter==options.end()){// new option
//                options.insert(topi);
//                utk_option_ret.emplace_back(topi, popped.first);
//            }
//        }
        bool new_option=False;
        bool newOption=False;
        if(popped.second->topk.size()==1){
            // if it is top1, push its adjacent vertex
            const vector<int> &top1_adj=ch_obj.get_neighbor_vertex(popped.second->topk.front());
            unordered_set<double> dss;
            vector<double> dsv;
            for (int adj_opt:top1_adj) {
                if(top1_calculated.find(adj_opt)!=top1_calculated.end()){
                    continue;
                }
                top1_calculated.insert(adj_opt);
                auto iter = top1_region.find(adj_opt);
//                assert(iter!=top1_region.end());
                if(iter==top1_region.end()){
                    continue; // precision problem occurs!
                }
                double dist=dist_region_w(parent_region,iter->second, w);
                if(dist!=INFINITY){
                    vector<int> tmp;
                    tmp.push_back(adj_opt);
                    vector<vector<double>> new_region(parent_region);
                    for (vector<double> &its_own_topr: iter->second) {
                        new_region.push_back(its_own_topr);
                    }
                    region *r=new region(tmp, new_region);
                    id_radius.emplace(dist, r);
                    cnt++;
                }
            }
        }
        if(true){
            vector<float> x=find_point_in_region(w, popped.second->cone);
            assert(!x.empty());
            // get topk based on x
            vector<int> rtopk=computeTopK(dim, PG, ch_obj.rskyband,x,k);
            for (int opt:rtopk) {
                auto iter=options.find(opt);
                if(iter==options.end()){// new option
                    options.insert(opt);
                    assert(popped.first>=0);
                    utk_option_ret.emplace_back(opt, popped.first);
                    if(true){
                        cout<<options.size()<<": "<<popped.second->cone.size()<<"," << popped.first << ", "<<utk_cones_ret.size()<<
                            ", "<<cnt <<"," << ch_obj.get_neighbor_vertex(popped.second->topk.front()).size()<<" # ";
                        cout<<popped.second->topk<<"\n";
                    }
                    newOption=True;
                }
            }
            if(newOption){
                popped.second->topk=rtopk;
                popped.second->setRadius(popped.first);
                utk_cones_ret.emplace_back(popped);
            }
        }
        if(popped.second->topk.size()==k){ // a region that don't need to be partitioned
            for (int opt:popped.second->topk) {
                auto iter=options.find(opt);
                if(iter==options.end()){// new option
                    options.insert(opt);
                    utk_option_ret.emplace_back(opt, popped.first);
                    cout << "radius: " << popped.first << "\n";
                    if(true){
                        cout<<options.size()<<": "<<popped.second->cone.size()<<"," << popped.first << ", "<<utk_cones_ret.size()<<
                            ", "<<cnt <<"," << ch_obj.get_neighbor_vertex(popped.second->topk.front()).size()<<" # ";
                        cout<<popped.second->topk<<"\n";
                    }
                    auto now = chrono::steady_clock::now();
                    chrono::duration<double> elapsed_seconds= now-begin;
                    cout << "time: " << options.size() << ", " << elapsed_seconds.count()<<"\n";
                    new_option=True;
                }
            }
            if(new_option){
                popped.second->setRadius(popped.first);
                utk_cones_ret.emplace_back(popped);
            }
        }
        else if(popped.second->cone.size()>k*30) { // ignore the cell by precision issue, TODO solve it
        }
        else{
            unordered_set<int> ch_upd_s;
            int m=0;
            for(int top: popped.second->topk){
                for(int adj: ch_obj.get_neighbor_vertex(top)){
                    ch_upd_s.insert(adj);
                }
                m=max(ch_obj.get_option_layer(top), m);
            }
            for(int mp1_opt: ch_obj.get_layer(m+1)){
                ch_upd_s.insert(mp1_opt);
            }
            for (int top: popped.second->topk) {
                ch_upd_s.erase(top);
            }
            vector<int> ch_upd(ch_upd_s.begin(), ch_upd_s.end());
            const int square_vertex_cnt=dim+1;
            vector<vector<double>> square_vertexes(square_vertex_cnt, vector<double>(dim));
            unordered_map<int, vector<vector<double>>> tops_region=top_region(ch_upd, PG, square_vertexes);
            for (int ch_upd_opt: ch_upd_s) {
                auto iter = tops_region.find(ch_upd_opt);
                if(iter!=tops_region.end()){
                    double dist=dist_region_w(popped.second->cone, iter->second, w);
                    if(dist!=INFINITY){
                        vector<int> tmp(popped.second->topk);
                        tmp.push_back(ch_upd_opt);
                        vector<vector<double>> new_region(popped.second->cone);
                        // TODO here is deep copy maybe faster if shadow copy
                        for (vector<double> &its_own_topr: iter->second) {
                            new_region.push_back(its_own_topr);
                        }
                        region *r=new region(tmp, new_region);
                        id_radius.emplace(dist, r);
                        cnt++;
                    }
                }
            }
        }

        if(popped.second->topk.size()!=k){
            delete(popped.second);
        }
        else if(!new_option && !newOption){
            delete(popped.second);   // to save mem
        }
    }
    cout<<"cnt: "<<cnt<<"\n";
    for (auto left:id_radius) {
        delete (left.second);
    }
    return cnt;
}

vector<vector<double>> points_to_halfspace(vector<vector<double>> &points){
    // be careful such that the norms are pointing out the convex cone
    // which means the convex cone is represented as
    // n_1 \cdot w <= 0
    // n_2 \cdot w <= 0
    // ...
    // n_a \cdot w <= 0
    int dim=points[0].size();
    vector<vector<double>> square_vertexes;
    square_vertexes.emplace_back(dim, 0.0);
    square_vertexes.emplace_back(dim, 1.0);

    string s = to_string(dim) + " " + to_string(points.size() + square_vertexes.size()) + " ";
    for (vector<double> & square_vertex : square_vertexes){
        for (float j : square_vertex){
            s += to_string(j) + " ";
        }
    }
    for(vector<double> &point: points){
        for (float i:point) {
            s += to_string(i) + " ";
        }
    }
    istringstream is(s);
    RboxPoints rbox;
    rbox.appendPoints(is);
    Qhull q(rbox, "QJ");
    qhull_user qu;
    return qu.get_cone_norms(q, points); // make sure the first of Qhull input is \vec{0}_{d}
}

void utk_basic(float **PointSet, int dim, vector<float> &w, Rtree* rtree, int X, int k,
               vector<pair<int, double>> &utk_option_ret,
               vector<pair<vector<int>, vector<vector<double>>>> &utk_cones_ret){

    // 2 return value
    // 1. array of <option, topk_radius>
    // 2. array of <topk, region>
    // "apply k=1"
//    test_build_qhull();
//    return;
    auto begin = chrono::steady_clock::now();
    auto now = chrono::steady_clock::now();
    chrono::duration<double> elapsed_seconds= now-begin;
    unknown_x_efficient get_next_obj(dim, 1, w, *rtree, PointSet);
    pair<int, float> next={-1, INFINITY};
    //fetch top X options
    cout<< "begin fetch top X"<<endl;
    vector<int> CH_1_X_opt;
    while(CH_1_X_opt.size() < X){  // fetch top X
        next=get_next_obj.get_next();
        CH_1_X_opt.push_back(next.first);
    }
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;

    cout<< elapsed_seconds.count() << " fetch top X finish\n";
    // qhull class in lib qhull
    const int square_vertex_cnt=dim+1;
    vector<vector<double>> square_vertexes(square_vertex_cnt, vector<double>(dim));

    // init qhull with top X options
    CH_1_X_opt=build_qhull(CH_1_X_opt, PointSet, square_vertexes);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;

    cout<< elapsed_seconds.count() << " first time build qhull finish\n";

    // a 3-d example of square_vertex, square_vertex_cnt=4
    // point 0: (max(points[:, 0]), 0, 0)
    // point 1: (0, max(points[:, 1]), 0)
    // point 2: (0, 0, max(points[:, 1]))
    // point 3: \vec{0}

    // rho_star is computed as when CH_1.size()=X
    int cnt=0;
    while(CH_1_X_opt.size() < X){ // while(CH_1.size<X)
        while(CH_1_X_opt.size() < X){
            next=get_next_obj.get_next();
            if(next.second==INFINITY){
                break;
            }
            update_square_vertexes(square_vertexes, PointSet[next.first], dim);
            CH_1_X_opt.push_back(next.first);
            cnt++;
        }
        CH_1_X_opt=build_qhull(CH_1_X_opt, PointSet, square_vertexes);
        cout<< cnt<<" rebuild qhull finish "<< CH_1_X_opt.size() <<endl;
        if(next.second==INFINITY){
            break;
        }
    }
    // for now, qhull_obj contains points 0~3 and convex hull vertexes
    float rho_star=next.second;

    cout << "init rho_star: "<<rho_star<<endl;
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << " finish rho_star compute\n";
    // use known X version code to fetch rskyband options,
    // bear in mind such that we init \rho as \rho_star and X as INFINITY
    vector<pair<long int, float>> interval;
    computeRho(dim, k, INFINITY, w, *rtree, PointSet, interval, rho_star);
    vector<int> rskyband_CS;
    for (pair<long int, float> &p:interval) {
        rskyband_CS.push_back(p.first);
    }
    cout<< "rskyband size: "<<rskyband_CS.size()<< "\n";
    for (int i = 0; i < rskyband_CS.size(); ++i) {
        for (int j = 0; j < dim; ++j) {
            cout<< PointSet[rskyband_CS[i]][j] << ",";
        }
        cout << "\n";
    }
    ch ch_obj(rskyband_CS, PointSet, dim);
    const vector<int>& top1_idxes=ch_obj.get_layer(1);
    vector<vector<double>> tmp;

    double rho_star_d=rho_star; // change from float to double
    for (vector<c_float> &e:g_r_domain_vec) {
        for (int i = 0; i < dim; ++i) {
            cout << e[i] <<", ";
        }
        cout <<"\n";
    }
    cout <<"\n";

    for (vector<c_float> &e:g_r_domain_vec) {
        double atc_rho=rho_star_d;
        for (int i = 0; i < e.size(); ++i) {
            if(e[i]<0){
                atc_rho=min(atc_rho, -w[i]/e[i]); // in case of w[i] + \rho * e[i] <0 or >1
            }
        }
        tmp.push_back(atc_rho*e+w);    }
    for (int i = 0; i < tmp.size(); ++i) {
        for (int j = 0; j < dim; ++j){
            cout << tmp[i][j] << ", ";
        }
        cout<<"\n";
    }
    cout <<"\n";


//    class region{
//    public:
//        vector<int> topk;
//        float radius;
//        vector<vector<float>> cone;
//    };
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count();
    cout<< " begin generate domain" << endl;
    vector<vector<double>> begin_region=points_to_halfspace(tmp);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count();
    for (int i = 0; i < begin_region.size(); ++i) {
        for (int j = 0; j < dim; ++j){
            cout << begin_region[i][j] << ", ";
        }
        cout<<"\n";
    }
    cout <<"\n";

    cout<< " end generate domain" << endl;
    multimap<double, region*> id_radius; // <radius, region>
    vector<int> init_topi;
    vector<int> init_neighbors;
    cout<< "starting recursively get top regions\n";
    topRegions(begin_region, CH_1_X_opt, ch_obj, id_radius, 0,  PointSet, dim,
                    k, init_topi, w, init_neighbors);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count();
    cout<< " finish recursively get top regions\n";

    // until X different options
//    vector<pair<int, float>> utk_option_ret;
//    vector<pair<vector<int>, vector<vector<float>>>> utk_cones_ret;
    assert(!id_radius.empty());
    _Rb_tree_iterator<pair<const double, region *>> iter=id_radius.begin();
    unordered_set<int> options;
    while(options.size()<X){
        for (int option_idx: iter->second->topk) {
            if(options.find(option_idx)==options.end()){ // new option
               options.insert(option_idx);
               utk_option_ret.emplace_back(option_idx, iter->first);
            }
        }
        utk_cones_ret.emplace_back(iter->second->topk, iter->second->cone);
        ++iter;
    }

}

int utk_efficient(float **PointSet, int dim, vector<float> &w, Rtree* rtree, int X, int k,
               vector<pair<int, double>> &utk_option_ret,
               vector<pair<double, region*>> &utk_cones_ret){

    // 2 return value
    // 1. array of <option, topk_radius>
    // 2. array of <topk, region>
    // "apply k=1"
//    test_build_qhull();
//    return;
    auto begin = chrono::steady_clock::now();
    auto now = chrono::steady_clock::now();
    chrono::duration<double> elapsed_seconds= now-begin;
    unknown_x_efficient get_next_obj(dim, 1, w, *rtree, PointSet);
    pair<int, float> next={-1, INFINITY};
    //fetch top X options
    cout<< "begin fetch top X"<<endl;
    vector<int> CH_1_X_opt;
    int top1=0;
    assert(X>0);
    bool top1f= false;
    while(CH_1_X_opt.size() < X){  // fetch top X
        next=get_next_obj.get_next();
        CH_1_X_opt.push_back(next.first);
        if(!top1f){
            top1=CH_1_X_opt.back();
            top1f=true;
        }
    }
    assert(top1!=0);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;

    cout<< elapsed_seconds.count() << " fetch top X finish\n";
    // qhull class in lib qhull
    const int square_vertex_cnt=dim+1;
    vector<vector<double>> square_vertexes(square_vertex_cnt, vector<double>(dim));

    // init qhull with top X options
    CH_1_X_opt=build_qhull(CH_1_X_opt, PointSet, square_vertexes);
//    Qhull q_test;
//    build_qhull(CH_1_X_opt, PointSet, square_vertexes, &q_test);
//    qhull_user qu;
//    vector<int> ch_test=qu.get_CH_pointID(q_test, CH_1_X_opt);
//    assert(ch_test.size()==CH_1_X_opt.size());

    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;

    cout<< elapsed_seconds.count() << " first time build qhull finish\n";

    // a 3-d example of square_vertex, square_vertex_cnt=4
    // point 0: (max(points[:, 0]), 0, 0)
    // point 1: (0, max(points[:, 1]), 0)
    // point 2: (0, 0, max(points[:, 1]))
    // point 3: \vec{0}

    // rho_star is computed as when CH_1.size()=X
    int cnt=0;
    while(CH_1_X_opt.size() < X){ // while(CH_1.size<X)
        while(CH_1_X_opt.size() < X){
            next=get_next_obj.get_next();
            if(next.second==INFINITY){
                break;
            }
            update_square_vertexes(square_vertexes, PointSet[next.first], dim);
            CH_1_X_opt.push_back(next.first);
            cnt++;
        }
        auto lbegin = chrono::steady_clock::now();

        CH_1_X_opt=build_qhull(CH_1_X_opt, PointSet, square_vertexes);
        auto lnow = chrono::steady_clock::now();
        chrono::duration<double> lelapsed_seconds= lnow-lbegin;
        cout << lelapsed_seconds.count()<<endl;
        cout<< cnt<<" rebuild qhull finish "<< CH_1_X_opt.size() <<endl;
        if(next.second==INFINITY){
            break;
        }
    }
    // for now, qhull_obj contains points 0~3 and convex hull vertexes
    float rho_star=next.second;
//    float rho_star=0.000001;

    cout << "init rho_star: "<<rho_star<<endl;
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << " finish rho_star compute\n";
    // use known X version code to fetch rskyband options,
    // bear in mind such that we init \rho as \rho_star and X as INFINITY
    vector<pair<long int, float>> interval;
    computeRho(dim, k, INFINITY, w, *rtree, PointSet, interval, rho_star);
    vector<int> rskyband_CS;
    for (pair<long int, float> &p:interval) {
        rskyband_CS.push_back(p.first);
    }
    cout<< elapsed_seconds.count() << " rskyband size: "<<rskyband_CS.size()<< "\n";
//    for (int i = 0; i < rskyband_CS.size(); ++i) {
//        for (int j = 0; j < dim; ++j) {
//            cout<< PointSet[rskyband_CS[i]][j] << ",";
//        }
//        cout << "\n";
//    }
    ch ch_obj(rskyband_CS, PointSet, dim);
    const vector<int>& top1_idxes=ch_obj.get_layer(1);
//    bool check_top1=False;
//    for (int idx:top1_idxes) {
//        if(idx==top1){
//            check_top1=True;
//            break;
//        }
//    }
//    assert(check_top1);
    vector<vector<double>> tmp;

    double rho_star_d=rho_star; // change from float to double
//    for (vector<c_float> &e:g_r_domain_vec) {
//        for (int i = 0; i < dim; ++i) {
//            cout << e[i] <<", ";
//        }
//        cout <<"\n";
//    }
//    cout <<"\n";

    for (vector<c_float> &e:g_r_domain_vec) {
        double atc_rho=rho_star_d;
        for (int i = 0; i < e.size(); ++i) {
            if(e[i]<0){
                atc_rho=min(atc_rho, -w[i]/e[i]); // in case of w[i] + \rho * e[i] <0 or >1
            }
        }
        tmp.push_back(atc_rho*e+w);
    }
//    for (int i = 0; i < tmp.size(); ++i) {
//        for (int j = 0; j < dim; ++j){
//            cout << tmp[i][j] << ", ";
//        }
//        cout<<"\n";
//    }
//    cout <<"\n";
//    class region{
//    public:
//        vector<int> topk;
//        float radius;
//        vector<vector<float>> cone;
//    };
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << " begin generate domain" << endl;
    vector<vector<double>> begin_region=points_to_halfspace(tmp);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() <<"finish generate domain \n";
//    for (int i = 0; i < begin_region.size(); ++i) {
//        for (int j = 0; j < dim; ++j){
//            cout << begin_region[i][j] << ", ";
//        }
//        cout<<"\n";
//    }
//    cout <<"\n";

    cout << " begin find top1 region" << endl;
    multimap<double, region*> id_radius; // <radius, region>
    vector<vector<double>> square_vertexes2(square_vertex_cnt, vector<double>(dim));
    unordered_map<int, vector<vector<double>>> top1_region=top_region(top1_idxes, PointSet, square_vertexes2);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << " finish find top1 region" << endl;
    auto iter=top1_region.find(top1);
    assert(iter != top1_region.end());
    vector<int> topi;
    topi.push_back(top1);
    unordered_set<int> top1_calculated;
    top1_calculated.insert(top1);
    region *r=new region(topi, iter->second);
    id_radius.emplace(dist_region_w(iter->second, w), r);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << "starting recursively get top regions\n";
//    for(vector<double> &i:top1_region[top1]){
//        double tmp=0;
//        for (int j = 0; j <dim ; ++j) {
//            tmp+=w[j]*i[j];
//        }
//        cout<<tmp<<endl;
//    }
//    cout<<endl;
//    for(vector<double> &i:begin_region){
//        double tmp=0;
//        for (int j = 0; j <dim ; ++j) {
//            tmp+=w[j]*i[j];
//        }
//        cout<<tmp<<endl;
//    }

    int ret=topRegions_efficient(begin_region, ch_obj, id_radius,  PointSet, dim, X,
               k,  w, top1_calculated, utk_option_ret, utk_cones_ret, top1_region);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count();
    cout<< " finish recursively get top regions\n";
    return ret;
}


vector<int> kskyband(float **PG, int cnt, int dim, int k){
    vector<long> idx(cnt);
    vector<int> ret;
    iota(idx.begin(), idx.end(), 1);
    for (int i = 1; i <=cnt ; ++i) {
//        bool dominatedByK(const int dimen, const float pt[], vector<long> &kskyband, float* PG[], int k);
        if(!dominatedByK_noSL(dim, PG[i], idx, PG, k+1)){
            ret.push_back(i);
        }
    }
    return ret;
}

int utk_efficient_anti(float **PointSet, int dim, vector<float> &w, Rtree* rtree, int X, int k,
                  vector<pair<int, double>> &utk_option_ret,
                  vector<pair<double, region*>> &utk_cones_ret){

    // A FASTER VERSION FOR anti data
    // 2 return value
    // 1. array of <option, topk_radius>
    // 2. array of <topk, region>
    // "apply k=1"
//    test_build_qhull();
//    return;
    auto begin = chrono::steady_clock::now();
    auto now = chrono::steady_clock::now();
    chrono::duration<double> elapsed_seconds= now-begin;
    //fetch top X options
    cout<< "begin fetch top X"<<endl;
    assert(X>0);
    // qhull class in lib qhull
    // use known X version code to fetch rskyband options,
    // bear in mind such that we init \rho as \rho_star and X as INFINITY
    vector<int> rskyband_CS=kskyband(PointSet, objCnt, dim, k);
    cout<< "finish kskyband"<<endl;
    int top1=1;
    double dmax=0;
    for (int i = 1; i < objCnt; ++i) {
        double dot=0;
        for (int j = 0; j < dim; ++j) {
            dot+=(PointSet[i][j]+SIDELEN)*w[j];
        }
        if(dot>dmax){
            dmax=dot;
            top1=i;
        }
    }
    cout<< elapsed_seconds.count() << " rskyband size: "<<rskyband_CS.size()<< "\n";
    ch ch_obj(rskyband_CS, PointSet, dim);
    const vector<int>& top1_idxes=ch_obj.get_layer(1);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << " begin generate domain" << endl;
    vector<vector<double>> begin_region;
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() <<"finish generate domain \n";
    cout << " begin find top1 region" << endl;
    multimap<double, region*> id_radius; // <radius, region>
    vector<vector<double>> square_vertexes2(dim+1, vector<double>(dim));
    cout<< "CH_1 size:" << top1_idxes.size()<<endl;
    unordered_map<int, vector<vector<double>>> top1_region=top_region(top1_idxes, PointSet, square_vertexes2);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << " finish find top1 region" << endl;
    auto iter=top1_region.find(top1);
    assert(iter != top1_region.end());
    vector<int> topi;
    topi.push_back(top1);
    unordered_set<int> top1_calculated;
    top1_calculated.insert(top1);
    region *r=new region(topi, iter->second);
    id_radius.emplace(dist_region_w(iter->second, w), r);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count() << "starting recursively get top regions\n";

    int ret=topRegions_efficient(begin_region, ch_obj, id_radius,  PointSet, dim, X,
                                 k,  w, top1_calculated, utk_option_ret, utk_cones_ret, top1_region);
    now = chrono::steady_clock::now();
    elapsed_seconds= now-begin;
    cout<< elapsed_seconds.count();
    cout<< " finish recursively get top regions\n";
    return ret;
}
