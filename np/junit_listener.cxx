/*
 * Copyright 2011-2012 Gregory Banks
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
 */
#include "np/util/common.hxx"
#include <sys/stat.h>
#include <libxml++/document.h>
#include "np/junit_listener.hxx"
#include "np/job.hxx"
#include "except.h"

namespace np {
using namespace std;
using namespace np::util;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
junit_listener_t::needs_stdout() const
{
    return true;
}

void
junit_listener_t::begin()
{
}

string
junit_listener_t::get_hostname() const
{
    char hostname[1024];
    int r = gethostname(hostname, sizeof(hostname));
    return (r < 0 ? string("localhost") : string(hostname));
}

void
junit_listener_t::end()
{
    string hostname = get_hostname();
    string timestamp = abs_format_iso8601(abs_now());

    // TODO: mkdir_p
    string directory = "reports";
    int r = mkdir(directory.c_str(), 0777);
    if (r < 0 && errno != EEXIST)
    {
	fprintf(stderr, "np: cannot make directory %s: %s\n",
		directory.c_str(), strerror(errno));
	return;
    }

    map<string, suite_t>::iterator sitr;
    for (sitr = suites_.begin() ; sitr != suites_.end() ; ++sitr)
    {
	const string &suitename = sitr->first;
	suite_t *s = &sitr->second;

	xmlpp::Document *xdoc = new xmlpp::Document();
	// If only there were a standard DTD URL...
	// instead we have a Schema from
	// http://windyroad.org/dl/OpenSource/JUnit.xsd
	xmlpp::Element *xsuite = xdoc->create_root_node("testsuite");
	xsuite->set_attribute("name", suitename);
	xsuite->set_attribute("failures", "0");
	xsuite->set_attribute("tests", dec(s->cases_.size()));
	xsuite->set_attribute("hostname", hostname);
	xsuite->set_attribute("timestamp", timestamp);

	xsuite->add_child("properties");

	unsigned int nerrs = 0;
	int64_t sns = 0;
	map<string, case_t>::iterator citr;
	string all_stdout;
	string all_stderr;
	for (citr = s->cases_.begin() ; citr != s->cases_.end() ; ++citr)
	{
	    const string &casename = citr->first;
	    case_t *c = &citr->second;

	    xmlpp::Element *xcase = xsuite->add_child("testcase");
	    xcase->set_attribute("name", casename);
	    // TODO: this is wrong
	    xcase->set_attribute("classname", casename);

	    sns += c->elapsed_;
	    xcase->set_attribute("time", rel_format(c->elapsed_));

	    if (c->event_)
	    {
		event_t *e = c->event_;
		xmlpp::Element *xerror = xcase->add_child("error");
		xerror->set_attribute("type", e->which_as_string());
		xerror->set_attribute("message", e->description);
		xerror->add_child_text(e->as_string() +
				       "\n" +
				       e->get_long_location());
	    }
	    if (c->result_ == R_FAIL)
		nerrs++;

	    if (c->stdout_ != "")
	    {
		all_stdout += string("===") + casename + string("===\n");
		all_stdout += c->stdout_;
	    }
	    if (c->stderr_ != "")
	    {
		all_stderr += string("===") + casename + string("===\n");
		all_stderr += c->stderr_;
	    }
	}
	xsuite->set_attribute("errors", dec(nerrs));
	xsuite->set_attribute("time", rel_format(sns));

	xsuite->add_child("system-out")->add_child_text(all_stdout);
	xsuite->add_child("system-err")->add_child_text(all_stderr);

	string filename = directory + string("/TEST-") + suitename + ".xml";

	try
	{
	    xdoc->write_to_file(filename, "UTF-8");
	}
	catch (const xmlpp::exception& ex)
	{
	    fprintf(stderr, "np: caught xmlpp exception writing file %s: %s\n",
		    filename.c_str(), ex.what());
	}
	delete xdoc;
    }
}

junit_listener_t::case_t *
junit_listener_t::find_case(const job_t *j)
{
    string suitename = j->get_node()->get_parent()->get_fullname();

    string jobname = j->as_string();
    int off = jobname.find(suitename);
    string casename(jobname, off+suitename.length()+1);

    return &suites_[suitename].cases_[casename];
}

void
junit_listener_t::begin_job(const job_t *j __attribute__((unused)))
{
}

void
junit_listener_t::end_job(const job_t *j, result_t res)
{
    case_t *c = find_case(j);
    c->result_ = res;
    c->elapsed_ = j->get_elapsed();
    c->stdout_ = j->get_stdout();
    c->stderr_ = j->get_stderr();
}

void
junit_listener_t::add_event(const job_t *j, const event_t *ev)
{
    case_t *c = find_case(j);
    if (ev->get_result() == R_FAIL && !c->event_)
	c->event_ = ev->clone();
}

junit_listener_t::case_t::~case_t()
{
    delete event_;
}

// close the namespace
};
