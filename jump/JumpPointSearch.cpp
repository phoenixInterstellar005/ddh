#include "JumpPointSearch.h"

#include "FlexibleAStar.h"
#include "Heuristic.h"
#include "JumpPointAbstraction.h"
#include "JumpPointExpansionPolicy.h"
#include "OfflineJumpPointLocator.h"
#include "OnlineJumpPointLocator.h"
#include "RecursiveJumpPointExpansionPolicy.h"

#include "timer.h"

JumpPointSearch::JumpPointSearch(bool _online, unsigned int _maxdepth, 
	Heuristic* _heuristic, mapAbstraction* _map) : searchAlgorithm(), 
	online(_online), maxdepth(_maxdepth), heuristic(_heuristic), map(_map)
{
	std::string algname;
	JumpPointLocator* jpl = 0;
	ExpansionPolicy* expander = 0;
	if(online)
	{
		jpl = new OnlineJumpPointLocator(map);
		algname.append("JPS");
	}	
	else
	{
		jpl = new OfflineJumpPointLocator(
				dynamic_cast<JumpPointAbstraction*>(map));
		algname.append("JPAS");
	}

	if(maxdepth == 0)
	{
		expander = new JumpPointExpansionPolicy(jpl);
	}
	else
	{
		algname.append("R");
		expander = new RecursiveJumpPointExpansionPolicy(jpl, maxdepth);
	}
	expander->verbose = verbose;

	astar = new FlexibleAStar(expander, heuristic);
	name = const_cast<char*>(algname.c_str());
}

JumpPointSearch::~JumpPointSearch()
{
	delete astar;
}

path* 
JumpPointSearch::getPath(graphAbstraction *aMap, node *start,
		node *goal, reservationProvider *rp)
{
	resetMetrics();
	astar->verbose = verbose;

	path* thepath = astar->getPath(aMap, start, goal, rp);
	if(thepath && maxdepth > 0)
	{
		addIntermediateNodes(thepath);
	}

	searchTime += astar->getSearchTime();
	nodesExpanded = astar->getNodesExpanded();
	nodesTouched = astar->getNodesTouched();
	nodesGenerated = astar->getNodesGenerated();

	return thepath;
}

void
JumpPointSearch::addIntermediateNodes(path* thepath)
{
	if(verbose)
		std::cout << "JumpPointSearch::addIntermediateNodes\n";

	RecursiveJumpPointExpansionPolicy* expander;
	expander = dynamic_cast<RecursiveJumpPointExpansionPolicy*>(
			astar->getExpander());
	assert(expander);

	Timer t;
	t.startTimer();
	while(thepath && thepath->next)
	{
		// re-expand thepath->n
		expander->expand(thepath->n);
		for(node* n = expander->first(); n != 0; n = expander->next())
		{
			// find thepath->next->n among its neighbours
			if(&*n == &*(thepath->next->n))
				break;
		}
		assert(expander->n());
		nodesExpanded++;

		// get the intermediate nodes that were skipped on the way to
		// thepath->next->n because their branching factor equaled 1.
		JumpInfo* info = expander->getJumpInfo();
		assert(info);
		if(verbose)
		{
			std::cout << "expanding ("<<thepath->n->getLabelL(kFirstData)
				<<", "<<thepath->n->getLabelL(kFirstData+1)<<") ";
			info->print(std::cout);
		}

		// insert each intermediate jump point into the path
		// (the last node == thepath->n, so skip it)
		for(unsigned int i=0; i < info->nodecount() - 1; i++)
		{
			path* newnext = new path(info->getNode(i), thepath->next);
			thepath->next = newnext;
			thepath = newnext;
		}
		thepath = thepath->next;
	}
	searchTime += t.endTimer();
}

void 
JumpPointSearch::resetMetrics()
{
	nodesExpanded = 0;
	nodesTouched = 0;
	nodesGenerated = 0;
	searchTime = 0;
}
