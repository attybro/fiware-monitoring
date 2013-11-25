/*
 * Copyright 2013 Telefónica I+D
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */


#include <string>
#include <sstream>
#include <cstdlib>
#include "metadata_parser.h"
#include "cppunit/TestResult.h"
#include "cppunit/TestFixture.h"
#include "cppunit/TextTestRunner.h"
#include "cppunit/BriefTestProgressListener.h"
#include "cppunit/extensions/HelperMacros.h"


using CppUnit::TestResult;
using CppUnit::TestFixture;
using CppUnit::TextTestRunner;
using CppUnit::BriefTestProgressListener;
using namespace std;


// test suite
class MetadataParserTest: public TestFixture
{
	static string valid_uuid;
	static string valid_zone;
	static string valid_metadata;
	static string incomplete_metadata;

	// C external function wrappers
	static bool parse_metadata(const string& data, string& uuid, string& zone);

	// tests
	void parse_fails_with_invalid_metadata();
	void parse_ok_empty_values_for_incomplete_metadata();
	void parse_ok_with_valid_metadata();

public:
	static void suiteSetUp();
	static void suiteTearDown();
	void setUp();
	void tearDown();
	CPPUNIT_TEST_SUITE(MetadataParserTest);
	CPPUNIT_TEST(parse_fails_with_invalid_metadata);
	CPPUNIT_TEST(parse_ok_empty_values_for_incomplete_metadata);
	CPPUNIT_TEST(parse_ok_with_valid_metadata);
	CPPUNIT_TEST_SUITE_END();
};


int main(void)
{
	TextTestRunner runner;
	BriefTestProgressListener progress;
	runner.eventManager().addListener(&progress);
	runner.addTest(MetadataParserTest::suite());
	MetadataParserTest::suiteSetUp();
	bool success = runner.run("", false, true, false);
	MetadataParserTest::suiteTearDown();
	return (success) ? EXIT_SUCCESS : EXIT_FAILURE;
}


///////////////////////////////////////////////////////


string	MetadataParserTest::valid_uuid;
string	MetadataParserTest::valid_zone;
string	MetadataParserTest::valid_metadata;
string	MetadataParserTest::incomplete_metadata;


bool MetadataParserTest::parse_metadata(const string& data, string& uuid, string& zone)
{
	host_metadata_t metadata;
	int error = ::parse_metadata(data.c_str(), &metadata);
	uuid.assign((metadata.uuid) ? metadata.uuid : "");
	zone.assign((metadata.availability_zone) ? metadata.availability_zone : "");
	free_metadata(&metadata);
	return (bool) error;
}


void MetadataParserTest::suiteSetUp()
{
	ostringstream buffer;

	valid_uuid	= "eb132e54-0c1b-49d1-a31c-4d0bf9aa9de1";
	valid_zone	= "nova";
	valid_metadata	= dynamic_cast<ostringstream&>(buffer
	<< "{"
	<<	"\"uuid\": \"" << valid_uuid << "\","
	<<	"\"availability_zone\": \"" << valid_zone << "\","
	<<	"\"hostname\": \"ubuntu.novalocal\","
	<<	"\"launch_index\": 0,"
	<<	"\"public_keys\": {"
	<<		"\"keypair\": \"ssh-rsa AAAAB3Nza......0Fq6sw== user@mail.com\""
	<<	"},"
	<<	"\"name\": \"ubuntu\""
	<< "}"
	).str();

	buffer.str("");
	incomplete_metadata = dynamic_cast<ostringstream&>(buffer
	<< "{"
	<<	"\"availability_zone\": \"" << valid_zone << "\","
	<<	"\"hostname\": \"ubuntu.novalocal\","
	<<	"\"name\": \"ubuntu\""
	<< "}"
	).str();
}


void MetadataParserTest::suiteTearDown()
{
}


void MetadataParserTest::setUp()
{
}


void MetadataParserTest::tearDown()
{
}


void MetadataParserTest::parse_fails_with_invalid_metadata()
{
	string parse_uuid, parse_zone;
	bool parse_error = parse_metadata("not-a-JSON", parse_uuid, parse_zone);
	CPPUNIT_ASSERT(parse_error);
}


void MetadataParserTest::parse_ok_empty_values_for_incomplete_metadata()
{
	string parse_uuid, parse_zone;
	bool parse_error = parse_metadata(incomplete_metadata, parse_uuid, parse_zone);
	CPPUNIT_ASSERT(!parse_error);
	CPPUNIT_ASSERT(parse_uuid.empty() || parse_zone.empty());
}


void MetadataParserTest::parse_ok_with_valid_metadata()
{
	string parse_uuid, parse_zone;
	bool parse_error = parse_metadata(valid_metadata, parse_uuid, parse_zone);
	CPPUNIT_ASSERT(!parse_error);
	CPPUNIT_ASSERT(parse_uuid == valid_uuid);
	CPPUNIT_ASSERT(parse_zone == valid_zone);
}