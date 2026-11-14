int menuLanguagePreview=-1;
void LocalizeUnionMenu(zCMenu* menu){
 if(!menu)return;
 for(int i=0;i<menu->m_listItems.GetNum();++i){auto* item=menu->m_listItems[i];if(!item)continue;
  for(const auto& text:i18n::MenuCatalogue)if(_stricmp(item->GetName().ToChar(),text.symbol)==0){
   const zSTRING value(Ui(text.id));item->m_parText[text.index]=value;
   if(text.index==0){item->ClrPrintwin();if(item->m_pInnerWindow)item->m_pInnerWindow->ClrPrintwin();}
   item->SetText(value,text.index,FALSE);
  }
 }
}
void PreviewUnionLanguage(zCMenu* menu){
 if(!menu||!zoptions||coopTextEditing)return;
 const int wanted=zoptions->ReadInt(kMenuSection,"Language",0)==1?1:0;
 if(menuLanguagePreview==wanted)return;menuLanguagePreview=wanted;
 i18n::Set(wanted?i18n::Language::German:i18n::Language::English);LocalizeUnionMenu(menu);
}
