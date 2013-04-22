/*
 * Copyright (c) 2013 Anthony Bau
 * MIT License: http://opensource.org/licenses/MIT
*/
var order = "timestamp DESC";
$(function() {
  $(window).resize(function() {
    var bottom = $("#bottom"),
        items = $("#items"),
        top = $("#top"),
        main = $("#main"),
        panel = $("#panel");
    top.height(main.outerHeight());
    main.width(0.7*top.width() - 52 - 5);
    panel.width(0.3*top.width() - 22 - 5);
    bottom.height(window.innerHeight - bottom.offset().top - 30);
    items.height(bottom.height() - 30);
  }).resize();

  Date.parse_sql = function(string) {
    var parts = string.split(/[^\d]/);
    return (new Date(parts[0], parts[1] - 1, parts[2], parts[3], parts[4], parts[5])).getTime();
  }

  function toReadable(UTC_string) {
    var elapse = (Date.now() + (new Date()).getTimezoneOffset()*60000 - Date.parse_sql(UTC_string))/1000;
    if (elapse >= 31536000) {
      var time = Math.floor(elapse / 31536000)
      return time + (time === 1 ? " year ago" : " years ago");
    }
    else if (elapse >= 2592000) {
      var time = Math.floor(elapse / 2592000);
      return time + (time === 1 ? " month ago" : " months ago");
    }
    else if (elapse >= 604800) {
      var time = Math.floor(elapse / 604800);
      return time + (time === 1 ? " week ago" : " weeks ago");
    }
    else if (elapse >= 86400) {
      var time = Math.floor(elapse / 86400);
      return time + (time === 1 ? " day ago" : " days ago");
    }
    else if (elapse >= 3600) {
      var time = Math.floor(elapse / 3600);
      return time + (time === 1 ? " hour ago" : " hours ago");
    }
    else if (elapse > 60) {
      var time = Math.floor(elapse / 60);
      return time + (time === 1 ? " minute ago" : " minutes ago");
    }
    else return Math.ceil(elapse) + (elapse <= 1 ? " second ago" : " seconds ago");
  }

  function reload() {
    $(".item").remove();
    $.ajax({
      url:"load?order_by=" + encodeURIComponent(order),
      dataType:"json"
    }).done(function(obj) {
      var bottom = $("#items");
      var data = obj.posts;
      for (var i = 0; i < data.length; i += 1) {
        var new_element = $("<div class=\"item\"></div>")
          .text(data[i][2])
          .attr({"rowid":data[i][0],"timestamp":data[i][1],"description":data[i][3], "upvotes":data[i][4], "downvotes":data[i][5],"codefiles":data[i][6], "title":data[i][2]})
          .append($("<div></div>").text(Math.abs(data[i][4] - data[i][5])).addClass((data[i][4] - data[i][5] > 0 ? "upvoted_item" : (data[i][4] === data[i][5] ? "neutral_item" : "downvoted_item"))))
          .append($("<div></div>").text(toReadable(data[i][1])).addClass("timestamp_readable"))
          .append($("<div></div>").addClass("prototype_icon").css("display",(data[i][6].length === 0 ? "none" : "block")).attr("title","This idea has a proof-of-concept."))
          .click(function() {
            var j_this = $(this),
                j_links = $("#prototypes"),
                codefiles = j_this.attr("codefiles").split(";");
            codefiles.shift();
            $(".prototype_link, .spacer").remove();
            $(".cursored").removeClass("cursored");
            j_this.addClass("cursored");
            $("#main").attr({"rowid":j_this.attr("rowid"),"timestamp":j_this.attr("timestamp")});
            $("#main_header").text(j_this.attr("title"));
            $("#main_body").text(j_this.attr("description"));
            $("#upvote_number").text(j_this.attr("upvotes"));
            $("#downvote_number").text(j_this.attr("downvotes"));
            if (codefiles.length === 0) {
              j_links.append($("<div class=\"spacer\"></div>").text("no proof-of-concept").css("height","25px"));
            }
            for (var i = 0; i < codefiles.length; i += 1) {
              j_links.append($("<div class=\"prototype\"><a class=\"prototype_link\" target=\"_blank\" href=\"file/" + codefiles[i] + "\">" + codefiles[i].split("/")[2] + "</a></div>"));
            }
          });
        if (new_element.attr("rowid") == $("#main").attr("rowid")) {
          new_element.addClass("cursored");
        }
        bottom.append(new_element);
        if (!$("#main").attr("rowid")) {
          $(".item").first().click();
        }
      }
      $(".item[rowid="+$("#main").attr("rowid")+"]").click();
    });
  }
  
  reload();
  
  
  $("#name, #description").each(function() { this.grayed = true});

  $("#name").focus(function() {
    j_this = $(this);
    if (j_this[0].grayed === true) {
      j_this[0].grayed = false;
      j_this.css("color","#000");
      j_this.val("");
    }
  }).blur(function() {
    j_this = $(this);
    if (j_this.val() === "") {
      j_this[0].grayed = true;
      j_this.css("color", "#888");
      j_this.val("title");
    }
  });
  $("#description").focus(function() {
    j_this = $(this);
    if (j_this[0].grayed === true) {
      j_this[0].grayed = false;
      j_this.css("color","#000");
      j_this.val("");
    }
  }).blur(function() {
    j_this = $(this);
    if (j_this.val() === "") {
      j_this[0].grayed = true;
      j_this.css("color", "#888");
      j_this.val("description");
    }
  });
  $("#submit_button").click(function() {
    if (!$("#name")[0].grayed) {
      $.ajax({
        "url":"submit?name=" + encodeURIComponent($("#name").val()) + "&description=" + encodeURIComponent($("#description").val())
      }).done(function() {
        reload();
        $("#name, #description").val("");
        $("#name, #description").blur();
      });
    }
  });
  $("#downvote").click(function() {
    $.ajax({
      url: "downvote?rowid=" + $("#main").attr("rowid")
    }).done(function() {
      reload();
      $("#downvote_number").text((Number($("#downvote_number").text())+1));
    });
  });
  $("#upvote").click(function() {
    $.ajax({
      url: "upvote?rowid=" + $("#main").attr("rowid")
    }).done(function() {
      reload();
      $("#upvote_number").html((Number($("#upvote_number").text())+1));
    });
  });
  $("#order_time").click(function() {
    $(".selected_order").removeClass("selected_order");
    $(this).addClass("selected_order");
    order = "timestamp DESC";
    reload();
  });
  $("#order_votes").click(function() {
    $(".selected_order").removeClass("selected_order");
    $(this).addClass("selected_order");
    order = "votes";
    reload();
  });
  $("#order_alphabetic").click(function() {
    $(".selected_order").removeClass("selected_order");
    $(this).addClass("selected_order");
    order = "name";
    reload();
  });
  $("#order_code").click(function() {
    $(".selected_order").removeClass("selected_order");
    $(this).addClass("selected_order");
    order = "code";
    reload();
  });
  var uploader = $("#upload").fineUploader({
    dragAndDrop:{
      disableDefaultDropzone:true
    },
    text:{
      uploadButton:"&oplus; upload proof-of-concept"
    },
    request:{
      params:{
        rowid:function() {
          return $("#main").attr("rowid");
        }
      },
      paramsInBody:false,
      endpoint:"save"
    }
  }).on("complete",reload);
});

