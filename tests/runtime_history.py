#!/usr/bin/env python3
"""Synthetic native-history UI and voice-wait regressions; no commercial assets."""
import argparse,hashlib,io,math,os,struct,subprocess,tempfile,unittest,wave,zlib
from pathlib import Path
from runtime_fixtures import Script,archive,png,read_png,p32
EXE=None
def icon():
 def chunk(k,b):return struct.pack('>I',len(b))+k+b+struct.pack('>I',zlib.crc32(k+b))
 rows=(b'\0'+bytes((240,20,20,255))*12+bytes((20,240,20,255))*12)*12
 color=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',24,12,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
 frames=p32(0,0,12,12,100,0,0,0)+p32(*([0]*8))+p32(12,0,12,12,100,0,0,0)
 return b'YKG000\0\0'+p32(64)+b'\0'*28+p32(64,len(color),0,0,64+len(color),len(frames))+color+frames
def voice():
 out=io.BytesIO()
 with wave.open(out,'wb') as w:
  w.setparams((1,2,44100,0,'NONE','not compressed'));w.writeframes(b''.join(struct.pack('<h',int(math.sin(i*.1)*10000)) for i in range(44100)))
 return out.getvalue()
class HistoryRuntime(unittest.TestCase):
 def setUp(self):self.tmp=tempfile.TemporaryDirectory(prefix='yukari-history-');self.root=Path(self.tmp.name);self.game=self.root/'game';self.game.mkdir();self.state=self.root/'state'
 def tearDown(self):self.tmp.cleanup()
 def run_game(self,s,actions):
  package=self.game/'data01.ykc';package.write_bytes(archive([('start.yks',s.finish()),('base.png',png(1024,576,(40,60,80,255))),('blank.png',png(1,1,(0,0,0,0))),('history.png',png(1024,576,(20,20,20,255))),('icons.ykg',icon()),('voice.wav',voice()),('default.ini',b'HistoryRowsMargin=10\n')]))
  before=hashlib.sha256(package.read_bytes()).digest();file=self.root/'actions';file.write_text('\n'.join(actions)+'\n');env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SDL_RENDER_DRIVER='software')
  result=subprocess.run([str(EXE),'play',str(self.game),'--fast','--mute','--seed','0','--state-dir',str(self.state),'--actions',str(file),'--trace','--max-ms','20000'],env=env,capture_output=True,text=True,timeout=30);log=result.stdout+result.stderr;self.assertEqual(result.returncode,0,log[-7000:]);self.assertEqual(hashlib.sha256(package.read_bytes()).digest(),before);return log
 def base(self):
  s=Script();s.call('GraphicLoad',0,'base.png',0,0);s.call('GraphicLoad',1,'blank.png',0,0);s.call('GraphicLoadHide',9,'history.png',0,0);s.call('GraphicLoadHide',10,'icons.ykg',0,0);s.call('DefaultFontSet',0,'Synthetic',20,400,20,20);s.call('DefaultFontSet',1,'Synthetic',30,400,30,30);s.call('TextWindowSet',1,50,50,700,250);s.call('FontSet',1,0);s.call('DefaultTextWindowGroup',1);s.call('HistoryWindowSet',9,200,100,700,200);s.call('HistorySoundIconSet',10,0,1);s.call('HistorySoundNumberSet',99);s.call('HistoryTextColor',0xff,0xff00);s.call('HistoryToVoiceContinue',0);s.call('AreaSetHistoryPageSize',1)
  for n,(font,text,voiced) in enumerate([(0,'FIRST',False),(1,'VOICE',True),(0,'LAST',False)]):
   s.assign('String',20,'voice.wav' if voiced else '');s.call('FontSet',1,font);s.call('StrOutNW',1,text);s.call('HistoryRecord',1);s.call('PF',1)
  return s
 def test_positions_colors_voice_hover_and_restore(self):
  s=self.base();s.call('KeyWait');s.call('ScriptExit');before=self.root/'before.png';normal=self.root/'normal.png';hover=self.root/'hover.png';head=self.root/'head.png';after=self.root/'after.png'
  log=self.run_game(s,[f'capture {before}','history',f'capture {normal}','hover 189 110',f'capture {hover}','click 205 105','history-action 10',f'capture {head}','history-close',f'capture {after}','advance'])
  self.assertIn('HISTORY_READY pages=3',log);self.assertIn('HISTORY_VOICE channel=99',log);self.assertEqual(before.read_bytes(),after.read_bytes(),'history closing changed parent scene');self.assertNotEqual(normal.read_bytes(),head.read_bytes(),'head navigation did not move pages');self.assertNotEqual(normal.read_bytes(),hover.read_bytes(),'normal and hovered voice icon did not differ')
  w,h,p=read_png(normal);self.assertEqual((w,h),(1024,576));self.assertEqual(p(800,300),(20,20,20),'configured background layer not displayed');self.assertTrue(any(g>100 and g>r*2 for y in range(100,135) for x in range(200,320) for r,g,b in [p(x,y)]),'voiced page did not use configured second color/font');self.assertTrue(any(r>100 and r>g*2 for y in range(140,170) for x in range(200,300) for r,g,b in [p(x,y)]),'ordinary page did not use first color')
 def test_historymode_zero_is_modal_and_next_advances(self):
  s=self.base();s.call('HistoryMode',0);s.assign('GlobalFlag',9000,1);s.call('KeyWait');s.call('HistoryExitToNext',1);s.call('KeyWait');s.assign('GlobalFlag',9001,1);s.call('ScriptExit')
  log=self.run_game(s,['history-close','advance','history','history-action 3','quit']);self.assertIn('HISTORY_CLOSED advance=1',log);saved=(self.state/'yukari-system.dat').read_text();self.assertIn('F 9000 1',saved);self.assertIn('F 9001 1',saved)
 def test_keyvoicewait_finishes_only_after_voice(self):
  s=Script();s.call('SoundGroupSet',10,99,99);s.call('SoundGroupAutomodeSkipSet',10,4);s.call('SoundLoad',99,'voice.wav');s.call('SoundPlay',99);s.call('KeyVoiceWait');s.assign('GlobalFlag',9002,1);s.call('ScriptExit');log=self.run_game(s,[]);self.assertIn('F 9002 1',(self.state/'yukari-system.dat').read_text());self.assertEqual(log.count('  KeyVoiceWait'),1)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True);args,remaining=parser.parse_known_args();EXE=args.exe.resolve();unittest.main(argv=[__file__,*remaining])
