// KeepTeamFlagAway.cpp

#include "bzfsAPI.h"

int isTeamNumVal(bz_eTeamType team) {
    if (team == eRogueTeam) {
        return 0;
    } else if (team == eRedTeam) {
        return 1;
    } else if (team == eGreenTeam) {
        return 2;
    } else if (team == eBlueTeam) {
        return 3;
    } else if (team == ePurpleTeam) {
        return 4;
    } else {
        return -1;
    } // while rogues are rarely included in CTF, we can at least give them geno capability.
}

bz_eTeamType flagToTeamValue(const char* flagType) {
    if (strcmp("R*", flagType) == 0) {
        return eRedTeam;
    } else if (strcmp("G*", flagType) == 0) {
        return eGreenTeam;
    } else if (strcmp("B*", flagType) == 0) {
        return eBlueTeam;
    } else if (strcmp("P*", flagType) == 0) {
        return ePurpleTeam;
    }  else {
        return eNoTeam;
    }
}

void killTeamByPlayer(bz_eTeamType team, int player) {
    //Loop with proper alloc/dealloc of playerlist, also base player records.
    bz_APIIntList *playerList = bz_newIntList();
    if (playerList) {
    
    bz_getPlayerIndexList(playerList);
    for ( unsigned int i = 0; i < playerList->size(); i++) {
    int targetID = (*playerList)[i];
    bz_BasePlayerRecord *playRec = bz_getPlayerByIndex ( targetID );

        if (playRec != NULL) {
            if ((playRec->spawned) && (playRec->team == team)) {
                bz_killPlayer(targetID, false, player);
            }
        }
        bz_freePlayerRecord(playRec);
    }
    // Originally deleted playerlist here.
    }
    bz_deleteIntList(playerList);
}

// Utility functions.
int checkRange(int min, int max, int amount);
int checkPlayerSlot(int player);

class KeepTeamFlagAway : public bz_Plugin
{
public:
  const char* Name(){return "KeepTeamFlagAway[0.0.0]";}
  void Init ( const char* /*config*/ );
  void Event(bz_EventData *eventData );
  void Cleanup ( void );
  int playerGrabbed[200];
  int timerCount[200][2];
  int count = 30;
  double timePassed;
  double timeInterval = 1.0;
  int currentCount = 0;
};

BZ_PLUGIN(KeepTeamFlagAway)

void KeepTeamFlagAway::Init (const char* commandLine) {
  for(int i=0;i<=199;i++){
    playerGrabbed[i]=0;
    timerCount[i][0]=0;
    timerCount[i][1]=0;
  }
  Register(bz_ePlayerJoinEvent);
  Register(bz_ePlayerPartEvent);
  Register(bz_ePlayerDieEvent);
  Register(bz_eFlagGrabbedEvent);
  Register(bz_eFlagDroppedEvent);
  Register(bz_eTickEvent);
}

void KeepTeamFlagAway::Cleanup (void) {
  Flush();
}

void KeepTeamFlagAway::Event(bz_EventData *eventData ){
  switch (eventData->eventType) {
    case bz_ePlayerJoinEvent: {
      bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;
      int player = joinData->playerID;
      // Check and reset player
      if (checkPlayerSlot(player) == 1) {
        playerGrabbed[player]=0;
        timerCount[player][0]=0;
        timerCount[player][1]=0;
      } 
    }break;

    case bz_ePlayerPartEvent: {
      bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;
      int player = partData->playerID;
      // Check and reset player
      if (checkPlayerSlot(player) == 1) {
        playerGrabbed[player]=0;
        timerCount[player][0]=0;
        timerCount[player][1]=0;
      } 
    }break;

    case bz_ePlayerDieEvent: {
      bz_PlayerDieEventData_V2* deathData = (bz_PlayerDieEventData_V2*)eventData;
      int player = deathData->playerID;
      int killer = deathData->killerID;
      // Check and reset player
      if (checkPlayerSlot(player) == 1) {
        playerGrabbed[player]=0;
        timerCount[player][0]=0;
        timerCount[player][1]=0;
      }
    }break;

    case bz_eFlagGrabbedEvent: {
      bz_FlagGrabbedEventData_V1* flagGrabData = (bz_FlagGrabbedEventData_V1*)eventData;
      int player = flagGrabData->playerID;
      if (checkPlayerSlot(player)==1) {
          timerCount[player][0]=0;
          timerCount[player][1]=0;
          playerGrabbed[player]=0;
          // Reset all again
          bz_eTeamType flagTeam = flagToTeamValue(flagGrabData->flagType);
          if (flagTeam != eNoTeam) {
            bz_eTeamType playerTeam = bz_getPlayerTeam(player);
            if (flagTeam != playerTeam) {
              playerGrabbed[player]=1;
              // If matched conditionals, set triggers for tick event loop.
            }
          }
      }   
    }break;

    case bz_eFlagDroppedEvent: {
      bz_FlagDroppedEventData_V1* flagDropData = (bz_FlagDroppedEventData_V1*)eventData;
      int player = flagDropData->playerID;
      // Check and reset player
      if (checkPlayerSlot(player)==1) {
          timerCount[player][0]=0;
          timerCount[player][1]=0;
          playerGrabbed[player]=0;
      }  
    }break;

     case bz_eTickEvent: {
        bz_TickEventData_V1* tickData = (bz_TickEventData_V1*)eventData;
        double currentTime = tickData->eventTime;
        if ((currentTime - timePassed) >= timeInterval) {
            // ^This is very reliable with timer operations.
            timePassed += 1.0;
            // Update timer.
            for (int i = 0; i <= 199; i++) {
                if(playerGrabbed[i]==1) {
                // ^this is if someone matched the conditionals.
                    timerCount[i][0] += 1;
                    // Update timer if matched.
                    if (timerCount[i][0] >= count) {
                        // updates match counter value.
                        if (timerCount[i][1] == 0) {
                            // ^Since we haven't reached second stage, it is zero.
                            bz_BasePlayerRecord *playRec = bz_getPlayerByIndex ( i );
                            if (playRec) {
                                if ((playRec->currentFlagID == -1) || (playRec->spawned == false)) {
                                    playerGrabbed[i]=0;
                                } else {
                                    // The reason we don't just grab the flag name, is since record offers non-abbre version
                                    bz_eTeamType teamSet = flagToTeamValue(bz_getFlagName(playRec->currentFlagID).c_str());
                                    // ^which is what is needed for flagToTeamValue to work.
                                    if ((teamSet != eNoTeam) && (playRec->team != teamSet)) {
                                        killTeamByPlayer(teamSet, i);
                                        timerCount[i][1] = timerCount[i][0];
                                    } else {
                                        playerGrabbed[i]=0;
                                    }
                                }
                            
                            }
                            bz_freePlayerRecord(playRec);
                            timerCount[i][0] = 0;
                        } else {
                            // if alive and with flag, cap if not rogue, else geno, else reset
                            bz_BasePlayerRecord *playRec = bz_getPlayerByIndex ( i );
                            if (playRec) {
                                if ((playRec->currentFlagID == -1) || (playRec->spawned == false)) {
                                    playerGrabbed[i]=0;
                                } else {
                                    // The reason we don't just grab the flag name, is since record offers non-abbre version
                                    bz_eTeamType teamSet = flagToTeamValue(bz_getFlagName(playRec->currentFlagID).c_str());
                                    // ^which is what is needed for flagToTeamValue to work.
                                    if ((teamSet != eNoTeam) && (playRec->team != teamSet)) {
                                        if (playRec->team != eRogueTeam) {
                                            bz_triggerFlagCapture(i, playRec->team, teamSet);
                                        } else {
                                            killTeamByPlayer(teamSet, i);
                                        }
                                    } else {
                                        playerGrabbed[i]=0;
                                    }
                                }
                            
                            }
                            bz_freePlayerRecord(playRec);
                        }
                    }
                    
                }
            }
        }  
     }break;

    default:{ 
    }break;
  }
}


int checkRange(int min, int max, int amount) {
    int num = 0;
    if ((amount >= min) && (amount <= max)) {
        num = 1;
    } else if ((amount < min) || (amount > max)) {
        num = 0;
    } else {
        num = -1;
    }
    return num;
}

int checkPlayerSlot(int player) {
    return checkRange(0,199,player); // 199 because of array.
}







