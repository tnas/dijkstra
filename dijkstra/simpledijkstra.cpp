/*
 * Copyright 2015 Thiago Nascimento <nascimenthiago@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <boost/heap/d_ary_heap.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/integer_traits.hpp>
#include <iostream>

#include "../common/dumbqueue.h"
#include "../common/dimacs.h"

using namespace std;
using boost::integer_traits;
using std::vector;


/// Data structure to store Key-Value pairs in a PriorityQueue
struct ValueKey {
   cost_t d;
   node_t u;
   ValueKey(cost_t _d, node_t _u)
      : d(_d), u(_u) {}
   /// The relation establishes the order in the PriorityQueue
   inline bool operator<(const ValueKey& rhs) const { return d < rhs.d; }
   inline bool operator>(const ValueKey& rhs) const { return d > rhs.d; }
   inline bool operator==(const ValueKey& rhs) const { return u == rhs.u; }
   inline void operator=(const ValueKey& rhs) { u = rhs.u; d = rhs.d; }
};


typedef boost::heap::d_ary_heap<ValueKey, boost::heap::arity<2>, boost::heap::mutable_<true> > BinaryHeap;
typedef boost::heap::fibonacci_heap<ValueKey> FibonacciHeap;
typedef DumbQueue<ValueKey> SimpleQueue;


/// Label for the labeling and/or dijkstra algorithm
enum Label { UNREACHED, LABELED, CLOSED };


/// Simple Arc class: store tuple (i,j,c)
class Arc {
   public:
      node_t    w;  /// Target node
      cost_t    c;  /// Cost of the arc
      /// Standard constructor
      Arc ( node_t _w, cost_t _c ) 
         : w(_w), c(_c) {}
};

/// Forward and Backward star: intrusive list
typedef vector<Arc> 	    FSArcList;
typedef FSArcList::iterator FSArcIter;


class Digraph {
  
   private:
     
      node_t  n_nodes;
      edge_t  m_edges;

      vector<FSArcList> adjacency_list;   

      /// Initialize distance vector with Infinity
      static const cost_t Inf = integer_traits<cost_t>::const_max;
      
   public:	
       
       Digraph(node_t _n, edge_t _m) : n_nodes(_n), m_edges(_m)
       {
            assert(n_nodes < Inf && m_edges < Inf);
            adjacency_list.reserve(n_nodes);
            
            /// Reserve memory for the set of arcs
            int avg_degree = m_edges/n_nodes+1;
            
            for ( int i = 0; i < n_nodes; ++i ) {
                FSArcList tmp;
                tmp.reserve(avg_degree);   
                adjacency_list.push_back(tmp);
            }      
        }
     
        node_t get_nnodes()
        {
            return n_nodes;
        }
      
        void addArc(node_t source_node, node_t target_node, cost_t cost) 
        {    
            adjacency_list[source_node].push_back(Arc(target_node, cost));
        }
      
        void print_adjacency_list() 
        {
	
            vector<FSArcList>::iterator it_nodes;
            node_t node = 0;
            
            for (it_nodes = adjacency_list.begin(); it_nodes < adjacency_list.end(); ++it_nodes) {
            
                cout << "from: " << node++ << " ==> ";
                
                FSArcIter it_arc;
                
                for (it_arc = (*it_nodes).begin(); it_arc < (*it_nodes).end(); ++it_arc) {
                    cout << "to:" << (*it_arc).w << " (" << (*it_arc).c << ") | ";
                }
                
                cout << "\n";
            }
        }
     
      void print_distance_vector(vector<node_t> path) 
      {
            for (vector<node_t>::iterator iter = path.begin(); iter != path.end(); ++iter)
            cout << *iter << " ";
            cout << "\n";
      }
     
      ///--------------------------------------------------
      /// Shortest Path for a graph with positive weights
      ///--------------------------------------------------
      template <typename PriorityQueue>
      cost_t shortest_path(node_t start_node, node_t end_node, vector<node_t>& previous) {    
	
         typedef typename PriorityQueue::handle_type     handle_t;
	 
         PriorityQueue     digraph_priority_queue;
         vector<handle_t>  distance_from_source(n_nodes);
         vector<Label>     node_status(n_nodes, UNREACHED);  
	 
         /// Initialize the source distance
         distance_from_source[start_node] = digraph_priority_queue.push(ValueKey(0, start_node));
         
         while (!digraph_priority_queue.empty()) {
	   
            ValueKey p = digraph_priority_queue.top();
            digraph_priority_queue.pop();
            node_t current_node  = p.u;
            node_status[current_node] = CLOSED;
            cost_t Du = -(*distance_from_source[current_node]).d;
	    
            if (current_node == end_node) { break; }
            
            /// For all edges (u, v) in E
            for (FSArcIter arc_iter = adjacency_list[current_node].begin(), it_end = adjacency_list[current_node].end(); arc_iter != it_end; ++arc_iter) {
	      
               node_t target_node = arc_iter->w;
	       
               if (node_status[target_node] != CLOSED) {
		 
                  cost_t Duv = arc_iter->c;
                  cost_t Dv  = Du + Duv;
		  
                  if (node_status[target_node] == UNREACHED) {
                     previous[target_node] = current_node;
                     node_status[target_node] = LABELED;
                     distance_from_source[target_node] = digraph_priority_queue.push(ValueKey(-Dv, target_node));
                  } 
                  else {
                     if (-(*distance_from_source[target_node]).d > Dv) {
                        previous[target_node] = current_node;
                        digraph_priority_queue.increase(distance_from_source[target_node], ValueKey(-Dv, target_node));
                     }
                  }
               }
            }
         }
         
         return -(*distance_from_source[end_node]).d;
      }
      
      template <typename CustomQueue>
      cost_t shortest_path_for_dummies(node_t start_node, node_t end_node, vector<node_t>& previous) {    
	
        CustomQueue      digraph_priority_queue;
        vector<ValueKey> distance_from_source(n_nodes, ValueKey(Inf, 0));
        vector<Label>    node_status(n_nodes, UNREACHED);  
	 
         /// Initialize the source distance
         distance_from_source[start_node] = digraph_priority_queue.push(ValueKey(0, start_node));
         
         while (!digraph_priority_queue.empty()) {
	   
            ValueKey p = digraph_priority_queue.top();
            digraph_priority_queue.pop();
            node_t current_node  = p.u;
            node_status[current_node] = CLOSED;
            cost_t Du = -(distance_from_source[current_node]).d;
	    
            if (current_node == end_node) { break; }
            
            /// For all edges (u, v) in E
            for (FSArcIter arc_iter = adjacency_list[current_node].begin(), it_end = adjacency_list[current_node].end(); arc_iter != it_end; ++arc_iter) {
	      
               node_t target_node = arc_iter->w;
	       
               if (node_status[target_node] != CLOSED) {
		 
                  cost_t Duv = arc_iter->c;
                  cost_t Dv  = Du + Duv;
		  
                  if (node_status[target_node] == UNREACHED) {
                     previous[target_node] = current_node;
                     node_status[target_node] = LABELED;
                     distance_from_source[target_node] = digraph_priority_queue.push(ValueKey(-Dv, target_node));
                  } 
                  else {
                     if (-(distance_from_source[target_node]).d > Dv) {
                        previous[target_node] = current_node;
                        digraph_priority_queue.increase(distance_from_source[target_node], ValueKey(-Dv, target_node));
                     }
                  }
               }
            }
         }
         
         return -(distance_from_source[end_node]).d;
      }
};
