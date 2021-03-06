#ifndef UTIL_GRAPH_H
#define	UTIL_GRAPH_H

#include <string>
#include <unordered_map>
#include <vector>

namespace cc 
{
namespace util 
{

class GraphPimpl;

/**
 * This class helps in creating a graph. The built graph can be written to the
 * output in several formats, like DOT or SVG. Since this implementation uses
 * GraphViz's representation, it is trivial to layout the graph with different
 * algorithms.
 */
class Graph
{
public:
  enum Format {DOT, SVG};
  
  struct Node     { std::string id; };
  struct Edge     { std::string id; };
  struct Subgraph { std::string id; };

  /**
   * By this constructor you can set the default properties of the graph. This
   * constructor only create a root graph (not subgraph).
   * @param name_ Name of the graph.
   * @param directed_ This sets whether the graph is directed or not.
   * @param strict_ The graph can be strict, which means that there are no
   * parallel and loop edges.
   */
  Graph(std::string name_ = "", bool directed_ = true, bool strict_ = false);
  
  ~Graph();
  
  /**
   * This static function converts a dot formatted graph to svg format.
   * @param graph_ Input graph in dot format
   * @return Svg format of input graph
   */
  static std::string dotToSvg(const std::string& graph_);
  
  /**
   * This function return whether the graph is directed.
   * @return True if the graph is directed; otherwise, false.
   */
  bool isDirected() const;
  
  /**
   * This function return whether the graph is strict.
   * @return True if the graph is strict; otherwise, false.
   */
  bool isStrict() const;
  
  /**
   * This function return whether the graph is a subgraph.
   * @return True if the graph is subgraph; otherwise, false.
   */
  bool isSubGraph() const;
  
  /**
   * This function returns the number of nodes in the graph.
   */
  int nodeCount() const;
  
  /**
   * This function returns the number of edges in the graph.
   */
  int edgeCount() const;
  
  /**
   * This function adds a node to the graph.
   * @param subgraph_ Subgraph to which the node will be added. If a default
   * constructed graph is given (i.e. its id is empty) then the node is added to
   * the root graph.
   * @return Node object.
   */
  Node addNode(Subgraph subgraph_ = Subgraph());
  
  /**
   * This function adds an edge to the graph.
   * @param from_ Source node.
   * @param to_ Target node.
   * @return Edge object
   */
  Edge addEdge(Node from_, Node to_);
  
  /**
   * This function adds a subgraph to the graph.
   * @param id_ Id of the subgraph. If empty, a unique id is generated by
   * default. If an id is given which is already used, then the function returns
   * the old subgraph, and doesn't add a new one.
   */
  Subgraph addSubgraph(std::string id_);
  
  /**
   * This function returns true if the given node (its identifier) exists.
   * @param node_
   * @return 
   */
  bool hasNode(Node node_) const;
  
  bool hasEdge(Node from_, Node to_) const;
  
  void delNode(Node node_);
  
  void delEdge(Node from_, Node to_);
  
  /**
   * This function sets the attributes of the graph. These attributes are listed
   * at this link: http://www.graphviz.org/content/attrs
   */
  void setAttribute(const std::string& key_, const std::string& value_);
  
  /**
   * This function sets the attributes of a node. These attributes are listed at
   * this link: http://www.graphviz.org/content/attrs
   */
  void setAttribute(const Node& node_,
                    const std::string& key_,
                    const std::string& value_);
  
  /**
   * This function sets the attributes of an edge. These attributes are listed
   * at this link: http://www.graphviz.org/content/attrs
   */
  void setAttribute(const Edge& edge_,
                    const std::string& key_,
                    const std::string& value_);
  
  /**
   * This function sets the attributes of a subgraph. These attributes are
   * listed at this link: http://www.graphviz.org/content/attrs
   */
  void setAttribute(const Subgraph& graph_,
                    const std::string& key_,
                    const std::string& value_);
  
  /**
   * This function copies the attributes of a node. These attributes are
   * listed at this link: http://www.graphviz.org/content/attrs
   */
  void setAttribute(const Node& targetNode_,
                    const Node& sourceNode_);
  
  /**
   * This function copies the attributes of an edge. These attributes are
   * listed at this link: http://www.graphviz.org/content/attrs
   */
  void setAttribute(const Edge& targetEdge_,
                    const Edge& sourceEdge_);
  
  /**
   * This function retrieves an attributes of a node. These attributes are
   * listed at this link: http://www.graphviz.org/content/attrs
   */
  std::string getAttribute(const Node& node_,
                           std::string key_);

  /**
   * This function retrieves an attributes of an edge. These attributes are
   * listed at this link: http://www.graphviz.org/content/attrs
   */
  std::string getAttribute(const Edge& edge_,
                           std::string key_);

  /**
   * This function generates the string representation of the graph in the
   * given format.
   */
  std::string output(Format format_) const;
  
  /**
   * This function returns the child nodes of a given node.
   */
  std::vector<Node> getChildren(const Node& node) const;
  
  /**
   * This function returns the parent nodes of a given node.
   */
  std::vector<Node> getParents(const Node& node) const;

private:
  /**
   * Copy constructor of the graph. This contructor doesn't do anything. We use
   * it so that nobody can copy the graph because of its inner representation.
   */
  Graph(const Graph& other_);
  
  /**
   * Assignment operator of the graph. This operator doesn't do anything. We use
   * it so that nobody can copy the graph because of its inner representation.
   */
  Graph& operator=(const Graph& other_);
  
  struct NodeHash
  {
    std::size_t operator()(const Node& node) const
    {
      return std::hash<std::string>()(node.id);
    }
  };

  /**
   * This function is used to generate a unique id for graph elements if needed.
   * The graph elements need a char* identifier.
   */
  std::string generateId();
  std::string currentId;
  
  GraphPimpl* _graphPimpl;
  
  bool _directed;
  bool _strict;
  bool _isSubgraph;
};

bool operator<(const Graph::Node& n1, const Graph::Node& n2);
bool operator<(const Graph::Edge& e1, const Graph::Edge& e2);
bool operator<(const Graph::Subgraph& s1, const Graph::Subgraph& s2);

bool operator==(const Graph::Node& n1, const Graph::Node& n2);
bool operator==(const Graph::Edge& e1, const Graph::Edge& e2);
bool operator==(const Graph::Subgraph& s1, const Graph::Subgraph& s2);

} // util
} // cc


#endif

