/**
 * vAlgo++ UI (for testing purpose)
 * Copyright (c) Abivin JSC
 * @file    Main browser-side code file
 * @version 0.1
 * @author  Dat Dinhquoc
 */

/**
 * Check valid url
 * http://stackoverflow.com/questions/8667070/
 * javascript-regular-expression-to-validate-url
 */
function is_valid_url(Url) {
  var Str = 
  "^(https?|ftp):\\/\\/(((([a-z]|\\d|-|\\.|_|~|[\\u00A0-\\uD7FF"+
  "\\uF900-\\uFDCF\\uFDF0-\\uFFEF])|(%[\\da-f]{2})|[!\\$&'\\(\\)"+
  "\\*\\+,;=]|:)*@)?(((\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])"+
  "\\.(\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d|[1-9]\\d|1"+
  "\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d|[1-9]\\d|1\\d\\d|2[0-4]"+
  "\\d|25[0-5]))|((([a-z]|\\d|[\\u00A0-\\uD7FF\\uF900-\\uFDCF"+
  "\\uFDF0-\\uFFEF])|(([a-z]|\\d|[\\u00A0-\\uD7FF\\uF900-\\uFDCF"+
  "\\uFDF0-\\uFFEF])([a-z]|\\d|-|\\.|_|~|[\\u00A0-\\uD7FF\\uF900-"+
  "\\uFDCF\\uFDF0-\\uFFEF])*([a-z]|\\d|[\\u00A0-\\uD7FF\\uF900-"+
  "\\uFDCF\\uFDF0-\\uFFEF])))\\.)+(([a-z]|[\\u00A0-\\uD7FF\\uF900-"+
  "\\uFDCF\\uFDF0-\\uFFEF])|(([a-z]|[\\u00A0-\\uD7FF\\uF900-"+
  "\\uFDCF\\uFDF0-\\uFFEF])([a-z]|\\d|-|\\.|_|~|[\\u00A0-\\uD7FF"+
  "\\uF900-\\uFDCF\\uFDF0-\\uFFEF])*([a-z]|[\\u00A0-\\uD7FF"+
  "\\uF900-\\uFDCF\\uFDF0-\\uFFEF])))\\.?)(:\\d*)?)(\\/((([a-z]|"+
  "\\d|-|\\.|_|~|[\\u00A0-\\uD7FF\\uF900-\\uFDCF\\uFDF0-"+
  "\\uFFEF])|(%[\\da-f]{2})|[!\\$&'\\(\\)\\*\\+,;=]|:|@)+("+
  "\\/(([a-z]|\\d|-|\\.|_|~|[\\u00A0-\\uD7FF\\uF900-\\uFDCF"+
  "\\uFDF0-\\uFFEF])|(%[\\da-f]{2})|[!\\$&'\\(\\)\\*"+
  "\\+,;=]|:|@)*)*)?)?(\\?((([a-z]|\\d|-|\\.|_|~|[\\u00A0-"+
  "\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFEF])|(%[\\da-f]{2})|[!"+
  "\\$&'\\(\\)\\*\\+,;=]|:|@)|[\\uE000-\\uF8FF]|\\/|\\?)*)?("+
  "\\#((([a-z]|\\d|-|\\.|_|~|[\\u00A0-\\uD7FF\\uF900-\\uFDCF"+
  "\\uFDF0-\\uFFEF])|(%[\\da-f]{2})|[!\\$&'\\(\\)\\*\\+,;=]|:|@)|"+
  "\\/|\\?)*)?$";

  var Regex = new RegExp(Str,"i");
  return Regex.test(Url);
}

/**
 * Add new web location
 */
function add_web_location() {
  var Full_Url     = $("#Nwl-Url").val();
  var Revisit_Time = $("#Nwl-Revisit-Time").val();

  //check if url is valid
  if (!is_valid_url(Full_Url)) {
    alert("URL is invalid!");
    return;
  }

  //check if revisit time is number
  if (!$.isNumeric(Revisit_Time) || Revisit_Time<=0) {
    alert("Revisit time must be positive number");
    return;
  }

  //send to server
  $.post("http://localhost:8891/webloc/add",JSON.stringify({
    Full_Url:     Full_Url,
    Revisit_Time: parseInt(Revisit_Time)
  })).
  done(function(Data){
    if (Data.Error) {
      alert("Error: "+JSON.stringify(Data.Error));
      return;
    }

    alert("Web location added successfully!");
  }).
  fail(function(Data){
    alert("Error: "+JSON.stringify(Data));
  });
}

/**
 * Entry point of web application
 */
$(function(){

  //get server status
  $.get("http://localhost:8891").
  done(function(Data){
    $("#Server-Status").html(Data);
  }).
  fail(function(Data){
    $("#Server-Status").html(JSON.stringify(Data));
  });
});

//end of file