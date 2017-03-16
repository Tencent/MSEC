/*
$( document ).ready(function() {

    $('body').append('<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">' +
        '<img src="/imgs/progress.gif" />' +
        '</div>');
});
*/
$(document).ajaxStart(function () {
    $("#div_loading").show();
});

$(document).ajaxComplete(function () {
    $("#div_loading").hide();
});

