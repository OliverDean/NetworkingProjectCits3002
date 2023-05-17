window.onload = function () {
  var form = document.getElementById("questionForm");

  // Create a single question from the file stream
  var questionFile = `10\nmcq\nWhat does pip stand for python?\nA. Pip Installs Python\nB. Pip Installs Packages\nC. Preferred Installer Program\nD. All of the mentioned\nC`;
  var lines = questionFile.split("\n");

  var question = {
    id: lines[0],
    type: lines[1],
    text: lines[2],
    options: lines.slice(3, lines.length - 1),
    answer: lines[lines.length - 1],
  };

  // Create a div for the question
  var questionDiv = document.createElement("div");
  questionDiv.className = "question-module multiple-choice";

  // Create a paragraph for the question text
  var questionText = document.createElement("p");
  questionText.textContent = question.text;
  questionDiv.appendChild(questionText);

  // Create radio buttons for each option
  for (var j = 0; j < question.options.length; j++) {
    var option = question.options[j];

    var optionDiv = document.createElement("div");
    optionDiv.className = "form-group";

    var input = document.createElement("input");
    input.type = "radio";
    input.id = "option" + (j + 1);
    input.name = "question1";
    input.value = option;
    optionDiv.appendChild(input);

    var label = document.createElement("label");
    label.for = input.id;
    label.textContent = option;
    optionDiv.appendChild(label);

    questionDiv.appendChild(optionDiv);
  }

  // Add the question div to the form
  form.appendChild(questionDiv);

  // Handle form submission event
  form.onsubmit = function (event) {
    event.preventDefault(); // Prevent the form from submitting normally

    // Create a new FormData object from the form
    var formData = new FormData(form);
    window.onload = function () {
      var form = document.getElementById("questionForm");
    
      // Create a single question from the file stream
      var questionFile = `10\nmcq\nWhat does pip stand for python?\nA. Pip Installs Python\nB. Pip Installs Packages\nC. Preferred Installer Program\nD. All of the mentioned\nC`;
      var lines = questionFile.split("\n");
    
      var question = {
        id: lines[0],
        type: lines[1],
        text: lines[2],
        options: lines.slice(3, lines.length - 1),
        answer: lines[lines.length - 1],
      };
    
      // Create a div for the question
      var questionDiv = document.createElement("div");
      questionDiv.className = "question-module multiple-choice";
    
      // Create a paragraph for the question text
      var questionText = document.createElement("p");
      questionText.textContent = question.text;
      questionDiv.appendChild(questionText);
    
      // Create radio buttons for each option
      for (var j = 0; j < question.options.length; j++) {
        var option = question.options[j];
    
        var optionDiv = document.createElement("div");
        optionDiv.className = "form-group";
    
        var input = document.createElement("input");
        input.type = "radio";
        input.id = "option" + (j + 1);
        input.name = "question1";
        input.value = option;
        optionDiv.appendChild(input);
    
        var label = document.createElement("label");
        label.for = input.id;
        label.textContent = option;
        optionDiv.appendChild(label);
    
        questionDiv.appendChild(optionDiv);
      }
    
      // Add the question div to the form
      form.appendChild(questionDiv);
    
      // Handle form submission event
      form.onsubmit = function (event) {
        event.preventDefault(); // Prevent the form from submitting normally
    
        // Create a new FormData object from the form
        var formData = new FormData(form);
    
        // Send a POST request to the server with the form data
        fetch("http://your-server-url.com/submit-answer", {
          method: "POST",
          body: formData,
        }).then(function (response) {
          if (response.ok) {
            alert("Answer submitted successfully!");
          } else {
            alert("Error: " + response.statusText);
          }
        });
      };
    };
  };
};