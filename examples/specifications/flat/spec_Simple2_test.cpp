// Copyright (c) 2020 JCA Electronics, Winnipeg, MB
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <string>     // std::string, std::to_string
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <typeinfo>       // operator typeid
#include <vector>
#include "hsm_test_utils.hpp"
#include <stdexcept>
#include <string>
#include <functional>

using std::vector;
using std::string;
using namespace testing;

//TODO add namespace

extern "C" {
  #include "hsm2.h"
  #include "spec_Simple.h"
}


struct SimContext {
  uint32_t time = 0;
  spec_Simple sm;
  Jxc _jxc = {};
  Jxc * const jxc = &_jxc;
};


extern "C" {
  void spec_Simple_xS1(void)   { RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_xS11(void)  { RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_t1(void)    { RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_tToS1(void) { RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_tToS11(void){ RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_eT1(void)   { RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_eT11(void)  { RecordingListener::current_listener->add_func_record(__func__); }
  void spec_Simple_eT111(void) { RecordingListener::current_listener->add_func_record(__func__); }
}

static string print(const vector<Record*>& records) {
  string result;
  
  int count = 0;

  for (auto record : records) {
    result += std::to_string(count) + ": " + record->toString() + "  \n  ";
    count++;
  }

  return result;
}


static void assert_equal(const vector<Record*>& expected, const vector<Record*>& actual)
{
  ASSERT_EQ(print(actual), print(expected));
}


static void assert_equal(const vector<Record*>& expected, const RecordingListener& recorder)
{
  assert_equal(expected, recorder.records);
}


static void assert_effect(SimContext& context, std::function<void(void)> func, const vector<Record*>& expected) {
  RecordingListener recorder(&context.sm.hsm);
  func();
  assert_equal(expected, recorder.records);
}


static void test_init(SimContext& context)
{
  SCOPED_TRACE("trace point");
  assert_effect(context, 
    [&](){ 
      spec_Simple_instance_init(context.jxc, &context.sm, "1"); 
    },
    {
      new EventRecord(spec_Simple_Root_ref,    HsmEventId__ENTER),
      new EventRecord(spec_Simple_Root_ref,    HsmEventId__LANDED_IN),
      new EventRecord(spec_Simple_ROOT__S_ref, HsmEventId__ENTER),
      new EventRecord(spec_Simple_ROOT__S_ref, HsmEventId__LANDED_IN),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_tToS1)),
    }
  );


  SCOPED_TRACE("trace point");
  assert_effect(context,
    [&](){ 
      spec_Simple_step(context.jxc, &context.sm);
    },
    {
      new EventRecord(spec_Simple_ROOT__S_ref,     HsmEventId__DO),
      new EventRecord(spec_Simple_ROOT__S__S1_ref, HsmEventId__ENTER),
      new EventRecord(spec_Simple_ROOT__S__S1_ref, HsmEventId__LANDED_IN),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_tToS11)),
    }
  );


  SCOPED_TRACE("trace point");
  assert_effect(context,
    [&](){ 
      spec_Simple_step(context.jxc, &context.sm);
    },
    {
      new EventRecord(spec_Simple_ROOT__S__S1_ref,      HsmEventId__DO),
      new EventRecord(spec_Simple_ROOT__S__S1__S11_ref, HsmEventId__ENTER),
      new EventRecord(spec_Simple_ROOT__S__S1__S11_ref, HsmEventId__LANDED_IN),
    }
  );
}



static void test_init_and_trans(SimContext& context)
{
  SCOPED_TRACE("trace point");
  assert_effect(context,
    [&](){ 
      spec_Simple_instance_init(context.jxc, &context.sm, "1"); 
    },
    {
      new EventRecord(spec_Simple_Root_ref,    HsmEventId__ENTER),
      new EventRecord(spec_Simple_Root_ref,    HsmEventId__LANDED_IN),
      new EventRecord(spec_Simple_ROOT__S_ref, HsmEventId__ENTER),
      new EventRecord(spec_Simple_ROOT__S_ref, HsmEventId__LANDED_IN),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_tToS1)),
    }
  );


  SCOPED_TRACE("trace point");
  assert_effect(context,
    [&](){ 
      spec_Simple_test_transitions(context.jxc, &context.sm, 3);
    },
    {
      new EventRecord(spec_Simple_ROOT__S_ref,     HsmEventId__TEST_TRANSITIONS),
      new EventRecord(spec_Simple_ROOT__S__S1_ref, HsmEventId__ENTER),
      new EventRecord(spec_Simple_ROOT__S__S1_ref, HsmEventId__LANDED_IN),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_tToS11)),
      //
      new EventRecord(spec_Simple_ROOT__S__S1_ref,      HsmEventId__TEST_TRANSITIONS),
      new EventRecord(spec_Simple_ROOT__S__S1__S11_ref, HsmEventId__ENTER),
      new EventRecord(spec_Simple_ROOT__S__S1__S11_ref, HsmEventId__LANDED_IN),
      //
      new EventRecord(spec_Simple_ROOT__S__S1__S11_ref, HsmEventId__TEST_TRANSITIONS),
      new EventRecord(spec_Simple_ROOT__S__S1_ref,      HsmEventId__TEST_TRANSITIONS),
      new EventRecord(spec_Simple_ROOT__S_ref,          HsmEventId__TEST_TRANSITIONS),
      new EventRecord(spec_Simple_Root_ref,             HsmEventId__TEST_TRANSITIONS),
    }
  );
}


static void test_s11_to_t111(SimContext& context)
{
  test_init(context);

  SCOPED_TRACE("trace point");
  assert_effect(context,
    [&](){ 
      spec_Simple_dispatch_event(context.jxc, &context.sm, spec_Simple_InputEvent_GO_EVENT); 
    },
    {
      new EventRecord(spec_Simple_ROOT__S__S1__S11_ref,          spec_Simple_InputEvent_GO_EVENT),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_t1)),
      //
      new EventRecord(       spec_Simple_ROOT__S__S1__S11_ref,   spec_Simple_InputEvent_EXIT),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_xS11)),
      //
      new EventRecord(           spec_Simple_ROOT__S__S1_ref,    spec_Simple_InputEvent_EXIT),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_xS1)),
      //
      new EventRecord(           spec_Simple_ROOT__S__T1_ref,    spec_Simple_InputEvent_ENTER),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_eT1)),
      //
      new EventRecord(       spec_Simple_ROOT__S__T1__T11_ref,   spec_Simple_InputEvent_ENTER),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_eT11)),
      //
      new EventRecord(  spec_Simple_ROOT__S__T1__T11__T111_ref,  spec_Simple_InputEvent_ENTER),
      new CalledFunctionRecord(STRINGIFY(spec_Simple_eT111)),
      new EventRecord(  spec_Simple_ROOT__S__T1__T11__T111_ref,  spec_Simple_InputEvent_LANDED_IN),

    }
  );

}


TEST(spec_Simple, InitAndStep) {
  SimContext context = { };
  test_init(context);
}


TEST(spec_Simple, InitAndTestTrans) {
  SimContext context = { };
  test_init_and_trans(context);
}


TEST(spec_Simple, test_s11_to_t111) {
  SimContext context = { };
  test_s11_to_t111(context);
}

