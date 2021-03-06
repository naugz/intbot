/**
 * vAlgo++ Smart Algorithm Service
 * Copyright (c) Abivin JSC
 * @file    Chatter class header file
 * @version 0.1
 * @author  Dat Dinhquoc
 */

//include guard begin
#ifndef ALGOS_CHATTING_CHATTER_HPP
#define ALGOS_CHATTING_CHATTER_HPP

//standard c++ headers
#include <map>
#include <vector>

//library headers
#include <mongocxx/client.hpp>

//in-project headers
#include <types.hpp>

//library namespaces
using namespace mongocxx;

//namespace path
namespace Algos {
namespace Chatting {

  /**
   * Chatter class
   */
  class chatter {

    //constants
    public:
      static string            INFORMATION_SUPPORT;
      static map<string,mapss> REFLECTS; //term reflection, eg. I-->you
      static map<string,mapss> WHATS; //used when a term is not understood
      static string            SUBJECT;
      static string            RELATION;
      static string            OBJECT;
      static string            TERM;

    //public reference properties
    public:
      client&   Db_Client;
      request&  Request;
      response& Response;

    //public properties
    public:
      string Motivation;
      string Language;
      string Name;

    //public constructor, destructor and methods
    public:
      chatter(client& Db_Client_,request& Request_,response& Response_);
      ~chatter();

      //thread methods
      bool           is_reply_for_motivation(string Reply);
      vector<string> split_text_into_sentences(string Text);
      string         tokens_to_term(vector<string> Tokens,long To_Before);
      bool           term_is_in_db(string Term);
      vector<string> get_terms_in_component(string Component);
      string         find_most_concerned_term(vector<string> Terms);
      string         get_reply_for_sentence(string Sentence);
      string         get_reply_for_text(string Text);
      string         check_reflection(string Token,string Type);
      string         what();
      void           set_concern(string Fragment);
      void           swap_concern(string Fragment);
      void           add_svo(string Fragment);
      void           add_sv(string Fragment);
      void           add_compounds(string Fragment);
      void           add_terms(string Fragment);
      void           add_terms_relations_concerns(string Text);
      void           run();
  };

//namespace path
}}

//include guard end
#endif

//end of file