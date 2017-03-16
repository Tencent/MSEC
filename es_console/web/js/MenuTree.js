

$(function(){
	$.fn.extend({
		MenuTree:function(options){
			

			var option = $.extend({
				click:function(a){ }
			},options);
			
			option.tree=this;
			
			option._init=function(){

				this.tree.find("ul ul").hide();
				this.tree.find("ul ul").prev("li").removeClass("open");
				
				this.tree.find("ul ul[show='true']").show();
				this.tree.find("ul ul[show='true']").prev("li").addClass("open");
			}
			

			this.find("a").click(function(){ $(this).parent("li").click(); return false; });
			

			this.find("li").click(function(){

				var a=$(this).find("a")[0];
				if(typeof(a)!="undefined")
					option.click(a);
				

				if($(this).next("ul").attr("show")=="true"){
					$(this).next("ul").attr("show","false");					
				}else{
					$(this).next("ul").attr("show","true");
				}
				

				option._init();
			});
			
			this.find("li").hover(
				function(){
					$(this).addClass("hover");
				},
				function(){
					$(this).removeClass("hover");
				}
			);
			

			this.find("ul").prev("li").addClass("folder");
			

			this.find("li").find("a").attr("hasChild",false);
			this.find("ul").prev("li").find("a").attr("hasChild",true);
			

			option._init();
			
		}
		
	});
});