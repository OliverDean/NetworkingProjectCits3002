window.onload = function () {
  var form = document.getElementById("codingQuestionForm");

  var xhr = new XMLHttpRequest();
  xhr.open("GET", "http://your-server-url.com/questionfile.txt", true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      var lines = xhr.responseText.split("\n");

      var question = {
        id: lines[0],
        type: lines[1],
        language: lines[2],
        text: lines[3],
        input: lines[4],
        output: lines[5],
      };
      //wnat to be able to get the correct question

      var questionDiv = document.createElement("div");
      questionDiv.className = "question-module coding";

      var questionText = document.createElement("p");
      questionText.textContent = question.text;
      questionDiv.appendChild(questionText);

      if (question.type === "programchallenge") {
        var answerDiv = document.createElement("div");
        answerDiv.className = "form-group";

        var label = document.createElement("label");
        label.for = "codingAnswer";
        label.textContent = "Answer:";
        answerDiv.appendChild(label);

        var textarea = document.createElement("textarea");
        textarea.id = "codingAnswer";
        textarea.name = "codingAnswer";
        textarea.rows = "4";
        textarea.required = true;
        answerDiv.appendChild(textarea);

        questionDiv.appendChild(answerDiv);
      }

      form.appendChild(questionDiv);

      form.onsubmit = function (event) {
        event.preventDefault();
        var formData = new FormData(form);
        formData.append("questionNumber", question.id);

        var submitXhr = new XMLHttpRequest();
        submitXhr.open("POST", "http://your-server-url.com/submit-answer", true);
        submitXhr.onreadystatechange = function () {
          if (submitXhr.readyState == 4) {
            if (submitXhr.status == 200) {
              alert("Answer submitted successfully!");
            } else {
              alert("Error: " + submitXhr.statusText);
            }
          }
        };
        submitXhr.send(formData);
      };
    }
  };
  xhr.send(null);
};
