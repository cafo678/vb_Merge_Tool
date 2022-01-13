# vb_Merge_Tool

Tool for merging togheter vb0 and vb2 files.  

They are frame analysis files dumped from the game that include data for the 3D models. 
VB0 is the file that contains the 3D data for each vertex, VB2 contains the weight data for each vertex.

The tool is ready for work with most of FFVII Remake files, but can be that it needs to be adpted to other files.

**How to use**  
Put the .exe in the same folder as the files to merge and run it, the only requirement is that the vb0 and the vb2 files are next to each other.  

Example:  
- *somefile.txt*
- *000285-vb0.txt*
- *000285-vb2.txt*
- *somefile.exe*
- *000294-vb0.txt*
- *000294vb2.txt*
- *ecc*

**How to adapt the tool to your needs**  
- Open the .sln
- Change the following 2 variables:
  - ``` const int32_t INDEXTOSKIP = 7; ``` Here the index of the Attribute to skip in the merge
  - ``` string Attributes[]{ "NORMAL", "TANGENT", "POSITION", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BLENDINDICES", "BLENDINDICES", "BLENDWEIGHT", "BLENDWEIGHT" }; ``` Here the list of the Attributes Names to set
  - Build

Thanks to **@Narknon**  and to the ** [OpenFF7R Discord Channel](discord.gg/qdxhFwT3Tr)**   
