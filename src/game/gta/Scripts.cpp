#include "utils.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/util/Joaat.hpp"
#include "core/backend/ScriptMgr.hpp"
namespace YimMenu::Utils {
    void ApplyForceToEntity(Entity entity, float x, float y, float z, float offX, float offY, float offZ)
    {
        ENTITY::APPLY_FORCE_TO_ENTITY(entity, 1, x, y, z, offX, offY, offZ, 0, 1, 1, 1, 0, 1);
    }

    void SetHeading(float heading) 
    {
          Ped ped = PLAYER::PLAYER_PED_ID();
          if (PED::IS_PED_IN_ANY_VEHICLE(ped, false))
          {
              Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(ped, false);
              ENTITY::SET_ENTITY_HEADING(veh, heading);
          }
          else 
          {
                ENTITY::SET_ENTITY_HEADING(ped, heading);
          }
     }

     void StartScript(const char* script_name, int stack_size)
     {
         Hash hash = Joaat(script_name);
         if (SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(hash) >= 1)
             return;
          SCRIPT::REQUEST_SCRIPT(script_name);
          while (!SCRIPT::HAS_SCRIPT_LOADED(script_name))
              ScriptMgr::Yield();
          BUILTIN::START_NEW_SCRIPT(script_name, stack_size);
          SCRIPT::SET_SCRIPT_AS_NO_LONGER_NEEDED(script_name);
          ScriptMgr::Yield(100ms);
          if (!strcmp(script_name, "appArcadeBusinessHub")) {
              while (SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(hash) >= 1) {
                  if (*ScriptGlobal(1971195).As<int*>() == -1) {
                      *ScriptGlobal(1951071).As<int*>() = 0;
                  }
                    ScriptMgr::Yield();
                }
            }
            else
            {
              while (SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(hash) > 0)
                   ScriptMgr::Yield();
                   *ScriptGlobal(1951071).As<int*>() = 0;
              }
         }
}
