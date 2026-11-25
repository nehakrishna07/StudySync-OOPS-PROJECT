#pragma once
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Small, dependency-free JSON value/parser for this teaching project.
// Rejects invalid JSON and supports Unicode escapes (including surrogate pairs).
class Json {
public:
 enum class Kind { Null, Bool, Number, String, Array, Object };
 using Object = std::map<std::string,Json>;
 using Array = std::vector<Json>;
private:
 Kind kind_ = Kind::Null;
 bool b_ = false; double number_ = 0;
 std::string string_; Array array_; Object object_;
 static void utf8(std::string& out,unsigned cp){
  if(cp<=0x7F)out+=char(cp);
  else if(cp<=0x7FF){out+=char(0xC0|(cp>>6));out+=char(0x80|(cp&63));}
  else if(cp<=0xFFFF){out+=char(0xE0|(cp>>12));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
  else{out+=char(0xF0|(cp>>18));out+=char(0x80|((cp>>12)&63));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
 }
 static std::string quote(const std::string&s){std::string out="\"";const char*hex="0123456789abcdef";for(unsigned char c:s){switch(c){case '"':out+="\\\"";break;case '\\':out+="\\\\";break;case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;default:if(c<32){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}}return out+'"';}
 class Parser {
  const std::string&s;size_t i=0;
  void ws(){while(i<s.size()&&std::isspace((unsigned char)s[i]))++i;}
  void need(char c){ws();if(i>=s.size()||s[i++]!=c)throw std::runtime_error("Invalid JSON syntax");}
  unsigned hex4(){unsigned n=0;for(int j=0;j<4;j++){if(i>=s.size())throw std::runtime_error("Invalid unicode escape");char c=s[i++];n=n*16+(c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:throw std::runtime_error("Invalid unicode escape"));}return n;}
  std::string str(){need('"');std::string out;while(i<s.size()){unsigned char c=s[i++];if(c=='"')return out;if(c<32)throw std::runtime_error("Control character in JSON string");if(c!='\\'){out+=char(c);continue;}if(i>=s.size())break;switch(c=s[i++]){case '"':out+='"';break;case '\\':out+='\\';break;case '/':out+='/';break;case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;case 'u':{unsigned cp=hex4();if(cp>=0xD800&&cp<=0xDBFF){if(i+2>s.size()||s.substr(i,2)!="\\u")throw std::runtime_error("Invalid surrogate");i+=2;unsigned low=hex4();if(low<0xDC00||low>0xDFFF)throw std::runtime_error("Invalid surrogate");cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);}else if(cp>=0xDC00&&cp<=0xDFFF)throw std::runtime_error("Invalid surrogate");utf8(out,cp);break;}default:throw std::runtime_error("Invalid string escape");}}throw std::runtime_error("Unterminated JSON string");}
  Json value(int depth){if(depth>64)throw std::runtime_error("JSON nested too deeply");ws();if(i>=s.size())throw std::runtime_error("Invalid JSON");char c=s[i];if(c=='"')return Json(str());if(c=='{'){++i;Object o;ws();if(i<s.size()&&s[i]=='}'){++i;return o;}while(true){ws();if(i>=s.size()||s[i]!='"')throw std::runtime_error("Object key expected");auto k=str();need(':');o[k]=value(depth+1);ws();if(i<s.size()&&s[i]=='}'){++i;break;}need(',');}return o;}if(c=='['){++i;Array a;ws();if(i<s.size()&&s[i]==']'){++i;return a;}while(true){a.push_back(value(depth+1));ws();if(i<s.size()&&s[i]==']'){++i;break;}need(',');}return a;}if(s.compare(i,4,"true")==0){i+=4;return true;}if(s.compare(i,5,"false")==0){i+=5;return false;}if(s.compare(i,4,"null")==0){i+=4;return Json();}size_t start=i;if(s[i]=='-')i++;if(i>=s.size())throw std::runtime_error("Invalid number");if(s[i]=='0')i++;else{if(!std::isdigit((unsigned char)s[i]))throw std::runtime_error("Invalid number");while(i<s.size()&&std::isdigit((unsigned char)s[i]))i++;}if(i<s.size()&&s[i]=='.'){i++;if(i>=s.size()||!std::isdigit((unsigned char)s[i]))throw std::runtime_error("Invalid number");while(i<s.size()&&std::isdigit((unsigned char)s[i]))i++;}if(i<s.size()&&(s[i]=='e'||s[i]=='E')){i++;if(i<s.size()&&(s[i]=='+'||s[i]=='-'))i++;if(i>=s.size()||!std::isdigit((unsigned char)s[i]))throw std::runtime_error("Invalid number");while(i<s.size()&&std::isdigit((unsigned char)s[i]))i++;}double v=std::stod(s.substr(start,i-start));if(!std::isfinite(v))throw std::runtime_error("Nonfinite JSON number");return v;}
 public:explicit Parser(const std::string&input):s(input){}Json parse(){Json v=value(0);ws();if(i!=s.size())throw std::runtime_error("Trailing JSON text");return v;}
 };
public:
 Json()=default;Json(std::nullptr_t){}Json(bool b):kind_(Kind::Bool),b_(b){}Json(int n):kind_(Kind::Number),number_(n){}Json(double n):kind_(Kind::Number),number_(n){}Json(const char*s):kind_(Kind::String),string_(s){}Json(std::string s):kind_(Kind::String),string_(std::move(s)){}Json(Array a):kind_(Kind::Array),array_(std::move(a)){}Json(Object o):kind_(Kind::Object),object_(std::move(o)){}
 static Json parse(const std::string&s){return Parser(s).parse();}
 bool isObject()const{return kind_==Kind::Object;}bool isArray()const{return kind_==Kind::Array;}bool isNumber()const{return kind_==Kind::Number;}
 const Json& at(const std::string&k)const{static Json nil;auto it=object_.find(k);return it==object_.end()?nil:it->second;}
 std::string str(const std::string&fallback="")const{return kind_==Kind::String?string_:fallback;}
 double num(double fallback=0)const{return kind_==Kind::Number?number_:fallback;}
 bool boolean(bool fallback=false)const{return kind_==Kind::Bool?b_:fallback;}
 const Array& array()const{return array_;}
 std::string dump()const{switch(kind_){case Kind::Null:return "null";case Kind::Bool:return b_?"true":"false";case Kind::Number:{std::ostringstream s;s<<std::setprecision(15)<<number_;return s.str();}case Kind::String:return quote(string_);case Kind::Array:{std::string s="[";for(const auto&v:array_){if(s.size()>1)s+=',';s+=v.dump();}return s+']';}case Kind::Object:{std::string s="{";for(const auto&[k,v]:object_){if(s.size()>1)s+=',';s+=quote(k)+":"+v.dump();}return s+'}';}}return "null";}
};
