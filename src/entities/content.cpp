/**
 * vAlgo++ Smart Algorithm Service
 * Copyright (c) Abivin JSC
 * @file     Content entity class implementation file
 * @version  0.1
 * @author   Dat Dinhquoc
 */

//library headers
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/operation_exception.hpp>

//in-project headers
#include <consts.hpp>
#include <entities/content.hpp>
#include <miscs/db.hpp>

//standard namespaces used
using namespace std;

//library namespaces
using namespace bsoncxx::builder::stream;
using namespace bsoncxx::document;
using namespace mongocxx;
using namespace mongocxx::result;

//in-project namespaces used
using namespace Entities;
using namespace Miscs;

/**
 * Content constructor
 */
content::content() {
  //
}

/**
 * Content desctructor
 */
content::~content() {
  //
}

/**
 * Save to db
 */
string content::save_to_db(client& Db_Client) {
  document Doc;
  Doc
  <<"_id"     <<this->Id
  <<"Url"     <<this->Url
  <<"Name"    <<this->Name
  <<"Title"   <<this->Title
  <<"Extract" <<this->Extract
  <<"Html"    <<this->Html;

  //links
  auto Array = Doc <<"Links" <<open_array;
  for (string Str: this->Links) {
    Array <<Str;
  }
  Array <<close_array;

  //insert
  try {
    db::insert_one(Db_Client,"contents",Doc);
    return OK;
  }
  catch (operation_exception& Exception) {
    return Exception.raw_server_error().value().
    view()["writeErrors"][0]["errmsg"].get_utf8().value.to_string();
  }
}

//end of file