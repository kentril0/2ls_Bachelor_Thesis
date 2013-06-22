/*******************************************************************\

Module: Indexing

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <util/xml.h>
#include <xmllang/xml_parser.h>

#include <goto-programs/read_goto_binary.h>
#include <goto-programs/goto_model.h>

#include "index.h"
#include "version.h"

/*******************************************************************\

Function: build_index

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void build_index(
  const std::vector<std::string> &files,
  const std::string &description,
  std::ostream &out,
  message_handlert &message_handler)
{
  out << "<?xml verion=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;

  out << "<DeltaCheckIndex version=\""
      << DELTACHECK_VERSION << "\">" << std::endl;
  
  out << "<description>";
  xmlt::escape(description, out);
  out << "</description>" << std::endl;
  out << std::endl;
  
  for(std::vector<std::string>::const_iterator
      it=files.begin();
      it!=files.end();
      it++)
  {
    out << "<file name=\"";
    xmlt::escape_attribute(*it, out);
    out << ">" << std::endl;

    goto_modelt goto_model;
    
    if(read_goto_binary(*it, goto_model, message_handler))
    {
      messaget message(message_handler);
      message.error() << "failed to read `" << *it << "'" << messaget::eom;
    }

    for(goto_functionst::function_mapt::const_iterator
        f_it=goto_model.goto_functions.function_map.begin();
        f_it!=goto_model.goto_functions.function_map.end();
        f_it++)
    {
      if(f_it->second.body_available)
      {
        out << "  <function id=\"";
        xmlt::escape_attribute(id2string(f_it->first), out);
        out << "/>" << std::endl;
      }
    }    
    
    out << "</file>" << std::endl;
  }
  
  out << "</DeltaCheckIndex>" << std::endl;  
}

/*******************************************************************\

Function: indext::read

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void indext::read(
  const std::string &in_file_name,
  message_handlert &message_handler)
{
  file_name=in_file_name;

  xmlt xml;
  parse_xml(in_file_name, message_handler, xml);
    
  xmlt::elementst::const_iterator DeltaCheckIndex=xml.find("DeltaCheckIndex");

  if(DeltaCheckIndex!=xml.elements.end())
  {
    description=DeltaCheckIndex->get_element("description");
  
    for(xmlt::elementst::const_iterator
        file_it=DeltaCheckIndex->elements.begin();
        file_it!=DeltaCheckIndex->elements.end();
        file_it++)
    {
      irep_idt file_name=file_it->get_attribute("name");
      
      files.insert(file_name);

      for(xmlt::elementst::const_iterator
          fkt_it=file_it->elements.begin();
          fkt_it!=file_it->elements.end();
          fkt_it++)
      {
        irep_idt id=fkt_it->get_attribute("id");
        functions.insert(id);
        function_to_file[id].insert(file_name);
        file_to_function[file_name].insert(id);
      }
    }
  }
}

#if 0
#include <fstream>

#include <util/config.h>
#include <util/xml.h>
#include <util/message.h>
#include <util/prefix.h>
#include <util/cprover_prefix.h>
#include <util/std_expr.h>
#include <util/xml_expr.h>

#include "get_goto_program.h"
#include "xml_conversion.h"
#include "summarization.h"
#include "dependencies.h"
#include "function_transformer.h"

//#include "cgraph_builder.h"
//#include "modular_fptr_analysis.h"
//#include "modular_globals_analysis.h"

/*******************************************************************\

Function: summarize_function_calls

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void summarize_function_calls_rec(
  const namespacet &ns,
  const goto_functionst &goto_functions,
  const exprt &function,
  std::set<irep_idt> &called_functions)
{
  if(function.id()==ID_symbol)
  {
    irep_idt id=to_symbol_expr(function).get_identifier();
    const symbolt &symbol=ns.lookup(id);
    if(!symbol.is_file_local)
      called_functions.insert(id);
  }
  else if(function.id()==ID_dereference)
  {
  }
  else if(function.id()==ID_if)
  {
  }
}
  
/*******************************************************************\

Function: summarize_function_calls

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void summarize_function_calls(
  const namespacet &ns, 
  const goto_functionst &goto_functions,
  const goto_functionst::goto_functiont &goto_function,
  std::ostream &out)
{
  std::set<irep_idt> called_functions;
  
  forall_goto_program_instructions(it, goto_function.body)
  {
    if(it->is_function_call())
    {
      const exprt &function=to_code_function_call(it->code).function();
      
      summarize_function_calls_rec(
        ns, goto_functions, function, called_functions);
    }
  }
  
  for(std::set<irep_idt>::const_iterator
      it=called_functions.begin();
      it!=called_functions.end();
      it++)
  {
    out << "  ";
    out << "<called id=\"";
    xmlt::escape_attribute(id2string(*it), out);
    out << "\"/>" << std::endl;
  }
}

/*******************************************************************\

Function: summarize_function

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void summarize_function(
  const namespacet &ns, 
  const goto_functionst &goto_functions,
  const symbolt &symbol,
  const goto_functionst::goto_functiont &goto_function,
  message_handlert &message_handler,
  std::ostream &out)
{
  out << "<function id=\"";
  xmlt::escape_attribute(id2string(symbol.name), out);
  out << "\">" << std::endl;
  
  if(symbol.location.is_not_nil() &&
     symbol.location.get_file()!="")
    out << "  " << xml(symbol.location);

  summarize_function_calls(ns, goto_functions, goto_function, out);
  
  function_transformer(ns, goto_functions, goto_function, message_handler, out);

  out << "</function>" << std::endl;
  out << std::endl;
}

/*******************************************************************\

Function: dump_exported_functions

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void dump_exported_functions(
  const namespacet &ns, 
  const goto_functionst &goto_functions,
  message_handlert &message_handler,
  std::ostream &out)
{
  out << "<functions>" << std::endl;

  // do this for each function
  forall_goto_functions(f_it, goto_functions)
  {
    if(!f_it->second.body_available)
      continue;

    if(has_prefix(id2string(f_it->first), CPROVER_PREFIX))
      continue;
  
    const symbolt &symbol=ns.lookup(f_it->first);
    
    if(symbol.is_file_local)
      continue;

    messaget message(message_handler);
    message.status() << "Summarizing " << f_it->first << messaget::eom;
  
    summarize_function(
      ns, goto_functions, symbol, f_it->second, message_handler, out);
  }
  
  out << "</functions>" << std::endl;
  out << std::endl;
}

/*******************************************************************\

Function: dump_state_variables

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void dump_state_variables(
  const symbol_tablet &symbol_table,
  std::ostream &out)
{
  out << "<state_variables>" << std::endl;

  forall_symbols(s_it, symbol_table.symbols)
  {
    const symbolt &symbol=s_it->second;
    
    if(has_prefix(id2string(symbol.name), CPROVER_PREFIX))
      continue;
      
    if(symbol.type.id()==ID_code ||
       symbol.is_type)
      continue;
      
    if(symbol.is_file_local)
      continue;
  
    out << "<state_variable id=\"";
    xmlt::escape_attribute(id2string(symbol.name), out);
    out << "\">" << std::endl;
  
    if(symbol.location.is_not_nil() && symbol.location.get_file()!="")
      out << xml(symbol.location);

    out << "</state_variable>" << std::endl;
  }
  
  out << "</state_variables>" << std::endl;
  out << std::endl;
}

/*******************************************************************\

Function: summarization

  Inputs:

 Outputs:

 Purpose: Phase I: produce a summary for a given file

\*******************************************************************/

void summarization(
  const function_file_mapt &function_file_map,
  const symbol_tablet &symbol_table,
  const goto_functionst &goto_functions,
  const optionst &options,
  message_handlert &message_handler,
  std::ostream &out)
{
  // first collect non-static function symbols that
  // have a body
  
  namespacet ns(symbol_table);
  
  dump_exported_functions(ns, goto_functions, message_handler, out);
  
  dump_state_variables(symbol_table, out);
  
  #if 0
  cgraph_buildert cg_builder;
  modular_fptr_analysist fptr_analysis;
  modular_globals_analysist globals_analysis;
  
  cg_builder.add_analysis(&fptr_analysis);
  cg_builder.add_analysis(&globals_analysis);
  
  cg_builder.analyze_module(symbol_table, goto_functions);
  cg_builder.serialize(file_name);
  #endif
}

/*******************************************************************\

Function: summarization

  Inputs:

 Outputs:

 Purpose: Phase I: produce a summary for a given file

\*******************************************************************/

void summarization(
  const function_file_mapt &function_file_map,
  const std::string &file_name,
  const optionst &options,
  message_handlert &message_handler)
{
  // first check dependencies
  if(!options.get_bool_option("force") &&
     dependencies(function_file_map, file_name, message_handler)==FRESH)
    return;

  // get the goto program
  symbol_tablet symbol_table;
  goto_functionst goto_functions;
  
  get_goto_program(file_name, options, symbol_table, goto_functions, message_handler);
  
  //goto_functions.output(ns, std::cout);

  std::string summary_file_name=file_name+".summary";
  std::ofstream summary_file(summary_file_name.c_str(),
    std::ios::binary|std::ios::trunc|std::ios::out);
  
  if(!summary_file)
    throw std::string("failed to write summary file");

  summary_file << "<summaries>" << std::endl;

  ::summarization(
    function_file_map,
    symbol_table,
    goto_functions,
    options,
    message_handler,
    summary_file);
  
  summary_file << "</summaries>" << std::endl;

  messaget message(message_handler);
  message.status() << "Summary written as " << summary_file_name << messaget::eom;
}

#endif
