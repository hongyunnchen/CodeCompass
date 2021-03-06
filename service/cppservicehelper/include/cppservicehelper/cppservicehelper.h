/*
 * cppexplorer.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ezoltbo
 */

#ifndef CPPSERVICEHELPER_CPPSERVICEHELPER_H
#define CPPSERVICEHELPER_CPPSERVICEHELPER_H

#include <memory>
#include <stdexcept>

#include <odb/database.hxx>
#include <odb/query.hxx>
#include <odb/result.hxx>

#include "model/cxx/cppastnode.h"
#include <model/cxx/cpptype.h>
#include <model/cxx/cppfunction.h>
#include <model/cxx/cppvariable.h>

#include "language-api/LanguageService.h"
#include "core-api/result_types.h"

#include "util/odbtransaction.h"

#include "odbquery.h"

namespace cc
{ 
namespace service
{  
namespace language
{
  
class CppServiceHelper
{
public:
  CppServiceHelper(const std::shared_ptr<odb::database>& db_);

  AstNodeInfo getAstNodeInfoByPosition(
    const core::FilePosition& fpos,
    const std::vector<std::string> & filters);

  InfoBox getInfoBox(
    const core::AstNodeId& astNodeId);
  
  InfoBox getInfoBoxByPosition(
    const core::FilePosition& fpos,
    const std::vector<std::string> & filters);

  AstNodeInfo getAstNodeInfo(const core::AstNodeId& astNodeId);

  std::vector<AstNodeInfo> getDefinition(const core::AstNodeId& astNodeId);

  std::string getDiagram(const core::AstNodeId& astNodeId,
    const core::DiagramId::type diagramType);

  std::string getLegend(const core::DiagramId::type diagramId);

  std::string getFileDiagram(const core::FileId& fileId,
    const core::DiagramId::type diagramType);
  
  /**
   * This function is implemented especially for function call path diagram
   * between two nodes.
   * @param astNodeId1 from this node
   * @param astNodeId2 to this node
   * @return svg representation of the graph
   */
  std::string getDiagram(const core::AstNodeId& astNodeId1,
    const core::AstNodeId& astNodeId2);
  
  std::vector<AstNodeInfo> getReferences(
    const core::AstNodeId& astNodeId);

  std::vector<AstNodeInfo> getReferences(
    const core::AstNodeId& astNodeId,
    const core::FileId& fileId);

  std::vector<AstNodeInfo> getCallerFunctions(
    const core::AstNodeId& astNodeId,
    const core::FileId& fileId);

  std::vector<AstNodeInfo> getFunctionAssigns(
    const core::AstNodeId& astNodeId,
    const core::FileId& fileId);

  core::RangedHitCountResult getReferencesPage(
    const core::AstNodeId& astNodeId,
    const int pageSize,
    const int pageNo);

  core::RangedHitCountResult getCallerFunctionsPage(
    const core::AstNodeId& astNodeId,
    const int pageSize,
    const int pageNo);

  core::RangedHitCountResult getFunctionAssignsPage(
    const core::AstNodeId& astNodeId,
    const int pageSize,
    const int pageNo);

  std::vector<core::AstNodeId> getTypeDefinitions(
    const std::string& path);

  std::vector<InfoNode> getInfoTree(
    const core::AstNodeId& astNodeId);

  std::vector<InfoNode> getSubInfoTree(
    const core::AstNodeId& astNodeId, const InfoQuery& infoQuery);

  std::vector<InfoNode> getInfoTreeForFile(const core::FileId&);

  std::vector<InfoNode> getSubInfoTreeForFile(const core::FileId&,
                                              const InfoQuery&);


  std::vector<AstNodeInfo> getFunctionCalls(
    const core::AstNodeId& astNodeId);

  std::shared_ptr<odb::database> getDatabase()
  {
    return db;
  }

  std::string getSourceCode(
    const core::AstNodeId& astNodeId);
  
  std::string getDocComment(
    const core::AstNodeId& astNodeId);

  /**
   * Get the symbolType of a specified AST node
   * @param astNodeId The node id to examine.
   * @return the symbolType of the AST node
   */
  model::CppAstNode::SymbolType getNodeKind(const core::AstNodeId& astNodeId);
  
  odb::result<model::CppAstNode> queryAstNodesByPosition(
    const core::FilePosition& fpos,
    const std::vector<std::string> & filters);

  /**
   * This function is tipically composed with queryAstNodesByPosition.
   * It selects the AST node which is clickable and has the smallest range
   * @param nodes The set we want to select from
   * @return the most appropriate AST node
   */
  model::CppAstNode selectProperAstNode(odb::result<model::CppAstNode> nodes);

  std::vector<model::CppAstNode> unique(
    odb::result<model::CppAstNode> original);
  
  
  void getBackwardSlicePos(
    std::vector<core::Range>& _return,
    const core::FilePosition& filePos);
  
  void getForwardSlicePos(
    std::vector<core::Range>& _return,
    const core::FilePosition& filePos);

  std::vector<core::SyntaxHighlight> getSyntaxHighlight(
    const core::FileId& fileId);
  
  model::HashType getInitialTypeOfTypedef(const model::HashType& td);

  std::vector<InfoNode> getCatalogue();

  std::vector<InfoNode> getSubCatalogue(const InfoQuery& query);

private:
  enum BuildDiagramRelation
  {
    NONE        = 0,
    CONTAINS    = 1,
    CONTAINEDBY = 2,
    PROVIDES    = 4,
    PROVIDEDBY  = 8,
    USES        = 16,
    USEDBY      = 32,
    IN          = CONTAINS | PROVIDES | USES,
    OUT         = CONTAINEDBY | PROVIDEDBY | USEDBY,
    ALL         = IN | OUT
  };
  
  std::vector<AstNodeInfo> getReferences(
    const core::AstNodeId& astNodeId,
    const std::unique_ptr<core::FileId>& fileId);

  std::string getClassDiagram(
    const core::AstNodeId& astNodeId);
  
  std::string getFullClassDiagram(
    const std::vector<core::AstNodeId>& astNodeIds);

  std::string getDirectoryFullClassDiagram(
    const model::FileId& fileId);
  
  std::string getClassCollaborationDiagram(
    const core::AstNodeId& astNodeId);
  
  std::string getFunctionCallDiagram(
    const core::AstNodeId& astNodeId);
  
  core::RangedHitCountResult makeRangedHitCountResult(
    odb::result<model::AstCountGroupByFiles> resultSet,
    const int pageSize, const int pageNo);

  std::string getDocCommentInTransaction(
    const model::HashType mangledNameHash);

  std::vector<InfoNode> collectCatalogueByHitCounter(
    odb::query<model::CppAstNode> query,
    const InfoQuery& iquery = {});

  std::shared_ptr<odb::database> db;

  util::OdbTransaction transaction;
  CppOdbQuery query;
};

} // language
} // service
} // cc

#endif // CPPSERVICEHELPER_CPPSERVICEHELPER_H

