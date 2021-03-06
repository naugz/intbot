/**
 * vAlgo++ Smart Algorithm Service
 * Copyright (c) Abivin JSC
 * @file    Crawler class implementation file
 * @version 0.1
 * @author  Dat Dinhquoc
 */

//standard c++ headers
#include <chrono>
#include <iostream>
#include <thread>
#include <typeinfo>
#include <vector>

//library headers
#include <client_http.hpp>
#include <boost/algorithm/string.hpp>
#include <bsoncxx/json.hpp>
#include <htmlcxx/html/ParserDom.h>
#include <mongocxx/client.hpp>
#include <mongocxx/cursor.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/query_exception.hpp>
#include <mongocxx/exception/operation_exception.hpp>

//in-project headers
#include <consts.hpp>
#include <types.hpp>
#include <server.hpp>
#include <entities/content.hpp>
#include <entities/moment.hpp>
#include <entities/webloc.hpp>
#include <miscs/db.hpp>
#include <miscs/utils.hpp>
#include <tasks/crawler.hpp>

//standard c++ namespaces
using namespace std;
using namespace std::chrono;

//library namespaces
using namespace boost;
using namespace bsoncxx::builder::stream;
using namespace bsoncxx::document;
using namespace htmlcxx;
using namespace htmlcxx::HTML;
using namespace mongocxx;
using namespace mongocxx::result;
using bsoncxx::from_json;
using bsoncxx::to_json;
using bsoncxx::types::b_date;

//namespaces being used
using namespace Entities;
using namespace Miscs;
using namespace Tasks;

/**
 * Default constructor
 */
crawler::crawler() {
  //
}

/**
 * Constructor with the index of thread
 */
crawler::crawler(long Thread_Index) {
  this->Thread_Index        = Thread_Index;
  this->Db_Client           = db::get_client();
  this->Is_Queuing          = false;
  this->Need_To_Clear_Queue = false;
}

/**
 * Empty webloc queue
 */
void crawler::clear_queue() {
  for (auto& Iterator: this->Queue) {
    try {
      delete Iterator.second;
    }
    catch (...) {
    } 
  }

  this->Queue.clear();
}

/**
 * Check if a web location is not yet queued in any crawler
 */
bool crawler::is_not_queued(webloc* Webloc) {
  return server::Singleton->is_not_queued(Webloc);
}

/**
 * Reschedule a webloc
 */
void crawler::reschedule_webloc(webloc* Webloc) {
  int64 New_Revisit_At = Webloc->Revisit_At + Webloc->Revisit_Time*60*60*1000;

  //document to find
  value Find_Value = document{}
  <<"_id" <<Webloc->Id
  <<finalize;

  //update as
  value Update_Value = document{} 
  <<"$set"
  <<open_document
    <<"Revisit_At" <<b_date(milliseconds(New_Revisit_At))
  <<close_document
  <<finalize;

  //do updating
  try {
    db::update_one(this->Db_Client,"weblocs",Find_Value,Update_Value);
  }
  catch (operation_exception& Exception) {
    cout <<"\nFailed to update " <<Webloc->Id <<endl;
  }
}

/**
 * Get text content inside a DOM node
 * http://stackoverflow.com/questions/5081181/htmlcxx-c-crawling-html
 */
string crawler::get_inner_text(const tree<Node>& Dom,
const tree<Node>::iterator& Parent) {
  string Result;

  for (long Index=0; Index<Dom.number_of_children(Parent); Index++) {
    tree<Node>::iterator It = Dom.child(Parent,Index);

    if (!It->isTag() && !It->isComment()) 
      Result += It->text();
  }

  return Result;
}

/**
 * Solve base url and add href to More_Links 
 */
void crawler::add_more_link(vector<string>& More_Links,webloc* Webloc,
string& Href) {

  //http
  if (Href.substr(0,7)=="http://") {
    More_Links.push_back(Href);
    return;
  }

  //https
  if (Href.substr(0,8)=="https://") {
    More_Links.push_back(Href);
    return;
  }

  //url from root
  if (Href[0]=='/') {
    string Link = Webloc->Protocol+"://"+Webloc->Domain_Name+":"+
    to_string(Webloc->Port)+Href;

    More_Links.push_back(Link);
    return;
  }

  //check whether path has trailing slash
  string Slash_Or_No;
  if (Webloc->Path.length()==0)
    Slash_Or_No = "/";
  else
  if (Webloc->Path[Webloc->Path.length()-1]=='/')
    Slash_Or_No = "";
  else
    Slash_Or_No = "/";

  //make the link
  string Link = Webloc->Protocol+"://"+Webloc->Domain_Name+":"+
  to_string(Webloc->Port)+Webloc->Path+Slash_Or_No+Href;

  //add to more links
  More_Links.push_back(Link);
}

/**
 * Crawl the current queue
 * Library: http://htmlcxx.sourceforge.net/
 */
void crawler::crawl_the_queue() {

  //more links to crawl
  vector<string> More_Links;
  
  //crawl all web locations in queue
  while (!this->Queue.empty()) {
    webloc* Webloc    = this->Queue.begin()->second;
    this->Current_Url = Webloc->Full_Url;

    //get web contents
    stringstream Out;
    Out <<"\nCrawler " <<this->Thread_Index<<", making HTTP GET request:"<<endl;
    Out <<Webloc->Domain_Name+":"+to_string(Webloc->Port) <<Webloc->Path <<endl;
    if (Webloc->Query_String.length()>0)
      Out <<Webloc->Query_String <<endl;
    else
      Out <<"NO_QUERY_STRING" <<endl;

    string Html;
    try {
      string Path_And_Query;
      if (Webloc->Query_String.length()>0)
        Path_And_Query = Webloc->Path+"?"+Webloc->Query_String;
      else
        Path_And_Query = Webloc->Path;

      Html = utils::http_get(
        Webloc->Domain_Name+":"+to_string(Webloc->Port),
        Path_And_Query
      );
      Out <<"Web page loaded OK" <<endl;
    }
    catch (invalid_argument& Error) {
      Html = "FAILED_TO_DO_HTTP_GET";
      Out <<"Error: " <<Error.what() <<endl;
    }

    //get the dom tree
    ParserDom Parser;
    tree<Node> Dom = Parser.parseTree(Html);
    
    //get title
    string Title("");
    for (auto Iterator=Dom.begin(); Iterator!=Dom.end(); Iterator++) {
      string Tag_Name(Iterator->tagName());

      if (Tag_Name=="title") {
        Title = this->get_inner_text(Dom,Iterator);
        break;
      }
    }

    //get extract
    string Extract("");
    for (auto Iter=Dom.begin(); Iter!=Dom.end(); Iter++) {
      string Tag_Name(Iter->tagName());

      //get text
      if (Tag_Name=="div" || Tag_Name=="p" || Tag_Name=="span" ||
      Tag_Name=="h1" || Tag_Name=="h2" || Tag_Name=="h3" || Tag_Name=="h4" ||
      Tag_Name=="h5" || Tag_Name=="h6" || Tag_Name=="b" || Tag_Name=="i" ||
      Tag_Name=="u" || Tag_Name=="a") {
        Extract += " "+this->get_inner_text(Dom,Iter);
        if (Extract.length()>EXTRACT_LENGTH)
          break;
      }
    }

    //get links
    vector<string> Links;
    for (auto Iter=Dom.begin(); Iter!=Dom.end(); Iter++) {
      string Tag_Name(Iter->tagName());

      if (Tag_Name=="a" || Tag_Name=="A") {
        Iter->parseAttributes();

        string Href = Iter->attribute(string("href")).second;
        Links.push_back(Href);
        this->add_more_link(More_Links,Webloc,Href);
      }
    }

    //make a content object
    trim(Extract);
    content Content;
    Content.Id      = Webloc->Full_Url;
    Content.Url     = Webloc->Full_Url;
    Content.Name    = Title;
    Content.Title   = Title;
    Content.Extract = Extract;
    Content.Html    = Html;
    Content.Links   = Links;

    //save content
    string Result = Content.save_to_db(this->Db_Client);
    if (Result==OK)
      Out <<"Saved." <<endl;
    else
      Out <<"Error: " <<Result <<endl;

    //log
    cout <<Out.str();
    cout.flush();

    //remove from queue
    auto Iter = this->Queue.find(Webloc->Full_Url);
    if (Iter!=this->Queue.end()) {
      delete Webloc;
      this->Queue.erase(Iter);
    }

    //check clear queue flag
    if (this->Need_To_Clear_Queue) {
      this->clear_queue();
      this->Need_To_Clear_Queue = false;

      stringstream Out;
      Out <<"\nCrawler " <<this->Thread_Index <<" cleared queue!" <<endl;
      cout <<Out.str();
      cout.flush();

      return;
    }
  }

  //add more links to the queue
  stringstream Out;
  for (string Url: More_Links) {
    webloc* Webloc = new webloc(Url,1); 

    if (Webloc->Is_Valid) {
      Webloc->save_to_db(this->Db_Client);
      this->Queue[Url] = Webloc;
    }
    else {
      Out <<"\nInvalid URL found:" <<endl;
      Out <<Url <<endl;
      delete Webloc;
    }
  }//for

  cout <<Out.str();
  cout.flush();
}//crawl the queue

/**
 * Thread method
 */
void crawler::run() {

  //run forever
  while (true) {

    //wait for something in the queue
    while (this->Queue.empty() || this->Is_Queuing) {
      //do nothing
    }

    //bulk printing thru' stringstream
    stringstream Out;
    Out <<"\nCrawler thread index " <<this->Thread_Index <<endl;
    Out <<"Crawling..." <<endl;
    cout <<Out.str();
    cout.flush();

    //do the real crawling
    this->crawl_the_queue();

    //log
    Out.str("");
    Out <<"\nCrawler thread index " <<this->Thread_Index <<endl;
    Out <<"Crawling done." <<endl;
    cout <<Out.str();
    cout.flush();
  }//while
}//run

//end of file