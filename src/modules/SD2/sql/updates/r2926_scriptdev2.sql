DELETE FROM script_texts WHERE entry BETWEEN -1004002 AND -1004000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1004000,'Yipe! Help Hogger!',0,1,0,0,'hogger SAY_CALL_HELP'),
(-1004001,'Hogger is eating! Stop him!',0,5,0,0,'hogger WHISPER_EATING'),
(-1004002,'No hurt Hogger!',0,1,0,0,'hogger SAY_HOGGER_BEATEN');
